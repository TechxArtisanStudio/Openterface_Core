#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "openterface/watchdog.h"

#define TICK_INTERVAL_MS 50u

static int failures = 0;

#define ASSERT_EQ_INT(expected, actual, msg) do { \
    if ((int)(expected) != (int)(actual)) { \
        fprintf(stderr, "FAIL: %s: expected %d, got %d\n", msg, (int)(expected), (int)(actual)); \
        failures++; \
    } \
} while (0)

#define ASSERT_EQ_STR(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, "FAIL: %s: expected '%s', got '%s'\n", msg, expected, actual); \
        failures++; \
    } \
} while (0)

/* ── mock transport ────────────────────────────────────────────────── */

typedef struct {
    int opened;
    int open_succeeds;        /* set to 0 to make next open() fail */
    int probe_succeeds;       /* set to 0 to make health probe fail */
    int open_attempts;
    int close_attempts;
    op_status_t probe_status; /* return value for health probe */
} mock_transport_context_t;

static op_status_t mock_open(void *context) {
    mock_transport_context_t *mock = (mock_transport_context_t *)context;
    mock->open_attempts++;
    if (!mock->open_succeeds) {
        return OP_STATUS_IO_ERROR;
    }
    mock->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t mock_close(void *context) {
    mock_transport_context_t *mock = (mock_transport_context_t *)context;
    mock->close_attempts++;
    mock->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t mock_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    (void)context; (void)buffer; (void)capacity; (void)out_length;
    return OP_STATUS_OK;
}

static op_status_t mock_write(void *context, const uint8_t *buffer, size_t length) {
    (void)context; (void)buffer; (void)length;
    return OP_STATUS_OK;
}

static op_status_t mock_control(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    (void)context; (void)request; (void)input; (void)input_length; (void)output; (void)output_capacity; (void)out_length;
    return OP_STATUS_NOT_SUPPORTED;
}

/* ── state tracking callback ──────────────────────────────────────── */

typedef struct {
    op_connection_state_t last_state;
    int state_change_count;
    op_connection_state_t states[32];
} state_tracker_t;

static void state_change_cb(const op_connection_event_t *event) {
    state_tracker_t *tracker = (state_tracker_t *)event->user_data;
    tracker->last_state = event->new_state;
    if (tracker->state_change_count < 32) {
        tracker->states[tracker->state_change_count] = event->new_state;
    }
    tracker->state_change_count++;
}

/* ── health probe ─────────────────────────────────────────────────── */

static op_status_t mock_health_probe(void *context) {
    mock_transport_context_t *mock = (mock_transport_context_t *)context;
    return mock->probe_succeeds ? OP_STATUS_OK : (mock->probe_status != OP_STATUS_OK ? mock->probe_status : OP_STATUS_IO_ERROR);
}

static void create_mock_transport(op_transport_t *transport, mock_transport_context_t *context) {
    static op_transport_vtable_t vtable = {
        mock_open, mock_close, mock_read, mock_write, mock_control
    };
    memset(context, 0, sizeof(*context));
    context->open_succeeds = 1;
    context->probe_succeeds = 1;
    context->probe_status = OP_STATUS_OK;
    op_transport_init(transport, OP_TRANSPORT_KIND_STUB, context, &vtable);
    (void)op_transport_open(transport);
}

static void create_watchdog_with_config(op_watchdog_t **wd, mock_transport_context_t *mock_ctx,
                                         uint32_t degrade_threshold, uint32_t recovery_threshold,
                                         state_tracker_t *tracker) {
    op_watchdog_config_t config = {0};
    config.transport = (op_transport_t *)mock_ctx; /* trick: we'll set real transport below */
    config.degrade_threshold = degrade_threshold;
    config.recovery_threshold = recovery_threshold;
    config.max_recovery_attempts = 3;
    config.health_check_interval_ms = 5000u;
    config.recovery_delay_ms = 200u;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = tracker;

    /* we need to pass a real transport — use a stub one */
}

/* ── test: initial state ──────────────────────────────────────────── */

static void test_initial_state(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 3;
    config.recovery_threshold = 5;
    config.max_recovery_attempts = 3;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "initial state is connected");
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(wd), "initial consecutive errors");
    ASSERT_EQ_INT(0, op_watchdog_total_errors(wd), "initial total errors");
    ASSERT_EQ_INT(0, op_watchdog_recovery_attempts(wd), "initial recovery attempts");

    op_watchdog_destroy(wd);
}

/* ── test: error accumulation → degraded ──────────────────────────── */

static void test_error_degrade_transition(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 3;
    config.recovery_threshold = 5;
    config.max_recovery_attempts = 3;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* 1-2 errors: still connected */
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "still connected after 2 errors");

    /* 3rd error: should trigger degrade */
    op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(wd), "degraded after 3 errors");
    ASSERT_EQ_INT(3, op_watchdog_consecutive_errors(wd), "consecutive errors = 3");

    /* 4th error: still degraded (not yet at recovery threshold) */
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(wd), "still degraded after 4 errors");

    /* verify state history */
    ASSERT_EQ_INT(1, tracker.state_change_count, "state changes: only connected→degraded");
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, tracker.last_state, "last state is degraded");

    op_watchdog_destroy(wd);
}

/* ── test: error accumulation → recovering → disconnected ─────────── */

static void test_error_recovery_disconnected(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 2;
    config.recovery_threshold = 3;
    config.max_recovery_attempts = 3;
    config.recovery_delay_ms = 200u;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* accumulate errors to trigger recovery */
    for (int i = 0; i < 3; i++) {
        op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(wd), "recovering after 3 errors");

    /* make open fail so recovery exhausts */
    mock.open_succeeds = 0;

    /* tick through recovery cycles */
    for (int i = 0; i < 50; i++) {
        op_watchdog_tick(wd, TICK_INTERVAL_MS);
    }

    ASSERT_EQ_INT(OP_CONN_STATE_DISCONNECTED, op_watchdog_get_state(wd), "disconnected after recovery exhausted");
    ASSERT_EQ_INT(3, op_watchdog_recovery_attempts(wd), "recovery attempts = max");

    op_watchdog_destroy(wd);
}

/* ── test: recovery succeeds ──────────────────────────────────────── */

static void test_recovery_succeeds(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 2;
    config.recovery_threshold = 3;
    config.max_recovery_attempts = 3;
    config.recovery_delay_ms = 200u;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* trigger recovery */
    for (int i = 0; i < 3; i++) {
        op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(wd), "recovering");

    /* transport will reopen successfully (default) */
    /* tick through recovery */
    for (int i = 0; i < 50; i++) {
        op_watchdog_tick(wd, TICK_INTERVAL_MS);
    }

    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "recovery succeeded, back to connected");
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(wd), "consecutive errors reset");

    op_watchdog_destroy(wd);
}

/* ── test: ok resets error counter ────────────────────────────────── */

static void test_ok_resets_errors(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 3;
    config.recovery_threshold = 5;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* 2 errors (close to degrade threshold) */
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(2, op_watchdog_consecutive_errors(wd), "2 consecutive errors");

    /* ok resets counter */
    op_watchdog_report_ok(wd);
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(wd), "errors reset to 0 after ok");
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "still connected");

    op_watchdog_destroy(wd);
}

/* ── test: degraded → ok → back to connected ──────────────────────── */

static void test_degraded_recover_via_ok(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 2;
    config.recovery_threshold = 5;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* trigger degraded */
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(wd), "degraded");

    /* ok brings back to connected */
    op_watchdog_report_ok(wd);
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "back to connected via ok");
    ASSERT_EQ_INT(2, tracker.state_change_count, "two state changes: degrade + recover");

    op_watchdog_destroy(wd);
}

/* ── test: force reconnect from disconnected ──────────────────────── */

static void test_force_reconnect(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 1;
    config.recovery_delay_ms = 200u;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* go to disconnected */
    op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    for (int i = 0; i < 20; i++) {
        op_watchdog_tick(wd, TICK_INTERVAL_MS);
    }
    /* after 2 errors (recovery_threshold) + 1 max recovery attempt, should be disconnected */
    ASSERT_EQ_INT(OP_CONN_STATE_DISCONNECTED, op_watchdog_get_state(wd), "disconnected");

    /* force reconnect */
    mock.open_succeeds = 1;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_force_reconnect(wd), "force reconnect ok");
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(wd), "state is recovering");

    /* tick through recovery */
    for (int i = 0; i < 50; i++) {
        op_watchdog_tick(wd, TICK_INTERVAL_MS);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(wd), "back to connected after force reconnect");

    op_watchdog_destroy(wd);
}

/* ── test: health probe failure triggers errors ───────────────────── */

static void test_health_probe_failure(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;
    state_tracker_t tracker = {0};

    memset(&tracker, 0, sizeof(tracker));

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.health_check_interval_ms = 100u; /* very short for test */
    config.health_probe = mock_health_probe;
    config.health_probe_context = &mock;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &tracker;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    /* make probe fail */
    mock.probe_succeeds = 0;
    mock.probe_status = OP_STATUS_TIMEOUT;

    /* tick to trigger health check */
    op_watchdog_tick(wd, 150u); /* > health_check_interval_ms */

    /* health probe failure should have been reported */
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(wd), "degraded from health probe failure");

    /* make probe succeed again */
    mock.probe_succeeds = 1;
    op_watchdog_tick(wd, 200u); /* trigger another health check */
    /* the health check will now succeed, but we need to verify */
    ASSERT_EQ_INT(1, op_watchdog_consecutive_errors(wd), "error count from first probe");

    op_watchdog_destroy(wd);
}

/* ── test: state labels ───────────────────────────────────────────── */

static void test_state_labels(void) {
    ASSERT_EQ_STR("connected", op_connection_state_label(OP_CONN_STATE_CONNECTED), "connected label");
    ASSERT_EQ_STR("degraded", op_connection_state_label(OP_CONN_STATE_DEGRADED), "degraded label");
    ASSERT_EQ_STR("recovering", op_connection_state_label(OP_CONN_STATE_RECOVERING), "recovering label");
    ASSERT_EQ_STR("disconnected", op_connection_state_label(OP_CONN_STATE_DISCONNECTED), "disconnected label");
}

/* ── test: null safety ────────────────────────────────────────────── */

static void test_null_safety(void) {
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_create(NULL, NULL), "null config");

    op_watchdog_tick(NULL, 100u);
    op_watchdog_report_error(NULL, OP_STATUS_IO_ERROR);
    op_watchdog_report_ok(NULL);
    ASSERT_EQ_INT(OP_CONN_STATE_DISCONNECTED, op_watchdog_get_state(NULL), "null state");
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(NULL), "null consecutive");
    ASSERT_EQ_INT(0, op_watchdog_total_errors(NULL), "null total");
    ASSERT_EQ_INT(0, op_watchdog_recovery_attempts(NULL), "null recovery");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_force_reconnect(NULL), "null force");
}

/* ── test: total errors persist across ok ─────────────────────────── */

static void test_total_errors_persist(void) {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd = NULL;

    create_mock_transport(&transport, &mock);

    op_watchdog_config_t config = {0};
    config.transport = &transport;
    config.degrade_threshold = 10;
    config.recovery_threshold = 20;
    config.state_cb = NULL;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &wd), "create watchdog");

    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_ok(wd); /* resets consecutive */

    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(wd), "consecutive reset");
    ASSERT_EQ_INT(2, op_watchdog_total_errors(wd), "total persists");

    op_watchdog_report_error(wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(1, op_watchdog_consecutive_errors(wd), "consecutive after ok");
    ASSERT_EQ_INT(3, op_watchdog_total_errors(wd), "total incremented");

    op_watchdog_destroy(wd);
}

int main(void) {
    test_initial_state();
    test_error_degrade_transition();
    test_error_recovery_disconnected();
    test_recovery_succeeds();
    test_ok_resets_errors();
    test_degraded_recover_via_ok();
    test_force_reconnect();
    test_health_probe_failure();
    test_state_labels();
    test_null_safety();
    test_total_errors_persist();

    if (failures != 0) {
        fprintf(stderr, "watchdog_test: %d failure(s)\n", failures);
        return 1;
    }

    printf("watchdog_test: all tests passed\n");
    return 0;
}
