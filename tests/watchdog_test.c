#include <stdio.h>
#include <string.h>

#include "openterface/watchdog.h"
#include "test_helpers.h"

#define TICK_INTERVAL_MS 50u

int test_failures = 0;

/* -- mock transport -------------------------------------------------- */

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

/* -- state tracking callback ---------------------------------------- */

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

/* -- health probe --------------------------------------------------─ */

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

/* -- test context: encapsulates all common state -------------------- */

typedef struct {
    op_transport_t transport;
    mock_transport_context_t mock;
    op_watchdog_t *wd;
    state_tracker_t tracker;
} watchdog_test_ctx_t;

/* init: zero state + create mock transport */
static void ctx_init(watchdog_test_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    create_mock_transport(&ctx->transport, &ctx->mock);
}

/* create_std: create watchdog with standard config (uses state_change_cb) */
static void ctx_create_std(watchdog_test_ctx_t *ctx,
                            uint32_t degrade, uint32_t recovery) {
    op_watchdog_config_t config = {0};
    config.transport = &ctx->transport;
    config.degrade_threshold = degrade;
    config.recovery_threshold = recovery;
    config.max_recovery_attempts = 3;
    config.recovery_delay_ms = 200u; /* short delay for fast test cycles */
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx->tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx->wd), "create watchdog");
}

/* destroy: cleanup watchdog */
static void ctx_destroy(watchdog_test_ctx_t *ctx) {
    if (ctx->wd) op_watchdog_destroy(ctx->wd);
}

/* -- test: initial state -------------------------------------------- */

static void test_initial_state(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 3, 5);

    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "initial state is connected");
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(ctx.wd), "initial consecutive errors");
    ASSERT_EQ_INT(0, op_watchdog_total_errors(ctx.wd), "initial total errors");
    ASSERT_EQ_INT(0, op_watchdog_recovery_attempts(ctx.wd), "initial recovery attempts");

    ctx_destroy(&ctx);
}

/* -- test: error accumulation → degraded ---------------------------- */

static void test_error_degrade_transition(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 3, 5);

    /* 1-2 errors: still connected */
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "still connected after 2 errors");

    /* 3rd error: should trigger degrade */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(ctx.wd), "degraded after 3 errors");
    ASSERT_EQ_INT(3, op_watchdog_consecutive_errors(ctx.wd), "consecutive errors = 3");

    /* 4th error: still degraded (not yet at recovery threshold) */
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(ctx.wd), "still degraded after 4 errors");

    /* verify state history */
    ASSERT_EQ_INT(1, ctx.tracker.state_change_count, "state changes: only connected→degraded");
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, ctx.tracker.last_state, "last state is degraded");

    ctx_destroy(&ctx);
}

/* -- test: error accumulation → recovering → disconnected ----------─ */

static void test_error_recovery_disconnected(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 2, 3);

    /* accumulate errors to trigger recovery */
    for (i = 0; i < 3; i++) {
        op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(ctx.wd), "recovering after 3 errors");

    /* make open fail so recovery exhausts */
    ctx.mock.open_succeeds = 0;

    /* tick through recovery cycles */
    for (i = 0; i < 50; i++) {
        op_watchdog_tick(ctx.wd, TICK_INTERVAL_MS);
    }

    ASSERT_EQ_INT(OP_CONN_STATE_DISCONNECTED, op_watchdog_get_state(ctx.wd), "disconnected after recovery exhausted");
    ASSERT_EQ_INT(3, op_watchdog_recovery_attempts(ctx.wd), "recovery attempts = max");

    ctx_destroy(&ctx);
}

/* -- test: recovery succeeds ---------------------------------------- */

static void test_recovery_succeeds(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 2, 3);

    /* trigger recovery */
    for (i = 0; i < 3; i++) {
        op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(ctx.wd), "recovering");

    /* transport will reopen successfully (default) */
    /* tick through recovery */
    for (i = 0; i < 50; i++) {
        op_watchdog_tick(ctx.wd, TICK_INTERVAL_MS);
    }

    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "recovery succeeded, back to connected");
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(ctx.wd), "consecutive errors reset");

    ctx_destroy(&ctx);
}

/* -- test: ok resets error counter ---------------------------------- */

static void test_ok_resets_errors(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 3, 5);

    /* 2 errors (close to degrade threshold) */
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(2, op_watchdog_consecutive_errors(ctx.wd), "2 consecutive errors");

    /* ok resets counter */
    op_watchdog_report_ok(ctx.wd);
    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(ctx.wd), "errors reset to 0 after ok");
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "still connected");

    ctx_destroy(&ctx);
}

/* -- test: degraded → ok → back to connected ------------------------ */

static void test_degraded_recover_via_ok(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 2, 5);

    /* trigger degraded */
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(ctx.wd), "degraded");

    /* ok brings back to connected */
    op_watchdog_report_ok(ctx.wd);
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "back to connected via ok");
    ASSERT_EQ_INT(2, ctx.tracker.state_change_count, "two state changes: degrade + recover");

    ctx_destroy(&ctx);
}

/* -- test: force reconnect from disconnected ------------------------ */

static void test_force_reconnect(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);

    /* custom config: max_recovery_attempts = 1 */
    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 1;
    config.recovery_delay_ms = 200u;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* make open fail so recovery fails */
    ctx.mock.open_succeeds = 0;

    /* go to disconnected */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    for (i = 0; i < 20; i++) {
        op_watchdog_tick(ctx.wd, TICK_INTERVAL_MS);
    }
    /* after 2 errors (recovery_threshold) + 1 max recovery attempt, should be disconnected */
    ASSERT_EQ_INT(OP_CONN_STATE_DISCONNECTED, op_watchdog_get_state(ctx.wd), "disconnected");

    /* force reconnect */
    ctx.mock.open_succeeds = 1;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_force_reconnect(ctx.wd), "force reconnect ok");
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(ctx.wd), "state is recovering");

    /* tick through recovery */
    for (i = 0; i < 50; i++) {
        op_watchdog_tick(ctx.wd, TICK_INTERVAL_MS);
    }
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "back to connected after force reconnect");

    ctx_destroy(&ctx);
}

/* -- test: health probe failure triggers errors --------------------─ */

static void test_health_probe_failure(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);

    /* custom config: health probe enabled */
    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.health_check_interval_ms = 100u; /* very short for test */
    config.health_probe = mock_health_probe;
    config.health_probe_context = &ctx.mock;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* make probe fail */
    ctx.mock.probe_succeeds = 0;
    ctx.mock.probe_status = OP_STATUS_TIMEOUT;

    /* tick to trigger health check */
    op_watchdog_tick(ctx.wd, 150u); /* > health_check_interval_ms */

    /* health probe failure should have been reported */
    ASSERT_EQ_INT(OP_CONN_STATE_DEGRADED, op_watchdog_get_state(ctx.wd), "degraded from health probe failure");

    /* make probe succeed again */
    ctx.mock.probe_succeeds = 1;
    op_watchdog_tick(ctx.wd, 200u); /* trigger another health check */
    /* the health check will now succeed, but we need to verify */
    ASSERT_EQ_INT(1, op_watchdog_consecutive_errors(ctx.wd), "error count from first probe");

    ctx_destroy(&ctx);
}

/* -- test: state labels --------------------------------------------─ */

static void test_state_labels(void) {
    ASSERT_EQ_STR("connected", op_connection_state_label(OP_CONN_STATE_CONNECTED), "connected label");
    ASSERT_EQ_STR("degraded", op_connection_state_label(OP_CONN_STATE_DEGRADED), "degraded label");
    ASSERT_EQ_STR("recovering", op_connection_state_label(OP_CONN_STATE_RECOVERING), "recovering label");
    ASSERT_EQ_STR("disconnected", op_connection_state_label(OP_CONN_STATE_DISCONNECTED), "disconnected label");
}

/* -- test: null safety ---------------------------------------------- */

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

    /* Backoff null safety */
    ASSERT_EQ_INT(0, op_watchdog_current_backoff_interval(NULL), "null backoff interval");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_get_backoff_config(NULL, NULL), "null get config");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_configure_backoff(NULL, NULL), "null configure backoff");
    op_watchdog_reset_backoff(NULL);  /* Should not crash */
}

/* -- test: total errors persist across ok --------------------------─ */

static void test_total_errors_persist(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);

    /* custom config: no callback, high thresholds */
    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 10;
    config.recovery_threshold = 20;
    config.state_cb = NULL;

    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_ok(ctx.wd); /* resets consecutive */

    ASSERT_EQ_INT(0, op_watchdog_consecutive_errors(ctx.wd), "consecutive reset");
    ASSERT_EQ_INT(2, op_watchdog_total_errors(ctx.wd), "total persists");

    op_watchdog_report_error(ctx.wd, OP_STATUS_TIMEOUT);
    ASSERT_EQ_INT(1, op_watchdog_consecutive_errors(ctx.wd), "consecutive after ok");
    ASSERT_EQ_INT(3, op_watchdog_total_errors(ctx.wd), "total incremented");

    ctx_destroy(&ctx);
}

/* -- test: exponential backoff configuration ------------------------- */

static void test_backoff_config(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);

    /* Test 1: With recovery_delay_ms set (backward compatibility) */
    ctx_create_std(&ctx, 2, 3);
    op_watchdog_backoff_config_t backoff_config;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_get_backoff_config(ctx.wd, &backoff_config), "get backoff config");
    /* When recovery_delay_ms is set but backoff is not, it should use recovery_delay_ms as fixed delay */
    ASSERT_EQ_INT(200, backoff_config.base_interval_ms, "base from recovery_delay_ms");
    ASSERT_EQ_INT(200, backoff_config.max_interval_ms, "max equals base (fixed delay)");
    ASSERT_EQ_FLOAT(1.0f, backoff_config.multiplier, 0.01f, "multiplier 1.0 (no backoff)");
    ASSERT_EQ_INT(200, op_watchdog_current_backoff_interval(ctx.wd), "initial backoff from recovery_delay_ms");
    ctx_destroy(&ctx);

    /* Test 2: With explicit backoff config */
    ctx_init(&ctx);
    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 2;
    config.recovery_threshold = 3;
    config.backoff.base_interval_ms = 500u;
    config.backoff.max_interval_ms = 5000u;
    config.backoff.multiplier = 2.0f;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_get_backoff_config(ctx.wd, &backoff_config), "get explicit config");
    ASSERT_EQ_INT(500, backoff_config.base_interval_ms, "explicit base");
    ASSERT_EQ_INT(5000, backoff_config.max_interval_ms, "explicit max");
    ASSERT_EQ_FLOAT(2.0f, backoff_config.multiplier, 0.01f, "explicit multiplier");
    ASSERT_EQ_INT(500, op_watchdog_current_backoff_interval(ctx.wd), "initial backoff from explicit config");
    ctx_destroy(&ctx);

    /* Test 3: Default values (no recovery_delay_ms, no backoff) */
    ctx_init(&ctx);
    op_watchdog_config_t default_config = {0};
    default_config.transport = &ctx.transport;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&default_config, &ctx.wd), "create with defaults");
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_get_backoff_config(ctx.wd, &backoff_config), "get default config");
    ASSERT_EQ_INT(OP_WATCHDOG_BACKOFF_DEFAULT_BASE_MS, backoff_config.base_interval_ms, "default base");
    ASSERT_EQ_INT(OP_WATCHDOG_BACKOFF_DEFAULT_MAX_MS, backoff_config.max_interval_ms, "default max");
    ASSERT_EQ_FLOAT(OP_WATCHDOG_BACKOFF_DEFAULT_MULTIPLIER, backoff_config.multiplier, 0.01f, "default multiplier");
    ctx_destroy(&ctx);
}

/* -- test: exponential backoff progression --------------------------- */

static void test_backoff_progression(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);

    /* Custom config with short delays for testing */
    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 5;
    config.backoff.base_interval_ms = 100u;
    config.backoff.max_interval_ms = 1000u;
    config.backoff.multiplier = 2.0f;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* Initial backoff should be 100ms */
    ASSERT_EQ_INT(100, op_watchdog_current_backoff_interval(ctx.wd), "initial backoff 100ms");

    /* Trigger recovery */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    ASSERT_EQ_INT(OP_CONN_STATE_RECOVERING, op_watchdog_get_state(ctx.wd), "recovering");

    /* Make open fail to increment backoff */
    ctx.mock.open_succeeds = 0;

    /* Tick through first recovery attempt */
    for (i = 0; i < 20; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }
    /* After first failure, backoff should increase to 200ms */
    ASSERT_EQ_INT(200, op_watchdog_current_backoff_interval(ctx.wd), "backoff after 1st failure");

    /* Continue ticking to trigger second attempt */
    for (i = 0; i < 30; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }
    /* After second failure, backoff should increase to 400ms */
    ASSERT_EQ_INT(400, op_watchdog_current_backoff_interval(ctx.wd), "backoff after 2nd failure");

    ctx_destroy(&ctx);
}

/* -- test: backoff reset on success ---------------------------------- */

static void test_backoff_reset_on_success(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);

    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 3;
    config.backoff.base_interval_ms = 100u;
    config.backoff.max_interval_ms = 1000u;
    config.backoff.multiplier = 2.0f;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* Trigger recovery */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);

    /* Make open fail once */
    ctx.mock.open_succeeds = 0;
    for (i = 0; i < 20; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }
    ASSERT_EQ_INT(200, op_watchdog_current_backoff_interval(ctx.wd), "backoff increased");

    /* Now make open succeed */
    ctx.mock.open_succeeds = 1;
    for (i = 0; i < 30; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }

    /* Backoff should reset to base */
    ASSERT_EQ_INT(100, op_watchdog_current_backoff_interval(ctx.wd), "backoff reset to base");
    ASSERT_EQ_INT(OP_CONN_STATE_CONNECTED, op_watchdog_get_state(ctx.wd), "back to connected");

    ctx_destroy(&ctx);
}

/* -- test: backoff max cap ------------------------------------------- */

static void test_backoff_max_cap(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);

    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 10;
    config.backoff.base_interval_ms = 100u;
    config.backoff.max_interval_ms = 500u; /* Low max for testing */
    config.backoff.multiplier = 10.0f;     /* High multiplier to hit max quickly */
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* Trigger recovery */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);

    /* Make open fail repeatedly */
    ctx.mock.open_succeeds = 0;

    /* After first failure: 100 * 10 = 1000, capped at 500 */
    for (i = 0; i < 20; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }
    ASSERT_EQ_INT(500, op_watchdog_current_backoff_interval(ctx.wd), "backoff capped at max");

    ctx_destroy(&ctx);
}

/* -- test: runtime backoff reconfiguration --------------------------- */

static void test_backoff_runtime_config(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 2, 3);

    /* Change backoff config at runtime */
    op_watchdog_backoff_config_t new_config = {
        .base_interval_ms = 500u,
        .max_interval_ms = 5000u,
        .multiplier = 3.0f
    };
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_configure_backoff(ctx.wd, &new_config), "configure backoff");

    /* Verify new config */
    op_watchdog_backoff_config_t retrieved_config;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_get_backoff_config(ctx.wd, &retrieved_config), "get config");
    ASSERT_EQ_INT(500, retrieved_config.base_interval_ms, "new base");
    ASSERT_EQ_INT(5000, retrieved_config.max_interval_ms, "new max");
    ASSERT_EQ_FLOAT(3.0f, retrieved_config.multiplier, 0.01f, "new multiplier");

    /* Current backoff should reflect new base */
    ASSERT_EQ_INT(500, op_watchdog_current_backoff_interval(ctx.wd), "current backoff updated");

    /* Reset to defaults */
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_configure_backoff(ctx.wd, NULL), "reset to defaults");
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_get_backoff_config(ctx.wd, &retrieved_config), "get default config");
    ASSERT_EQ_INT(OP_WATCHDOG_BACKOFF_DEFAULT_BASE_MS, retrieved_config.base_interval_ms, "default base restored");

    ctx_destroy(&ctx);
}

/* -- test: manual backoff reset -------------------------------------- */

static void test_backoff_manual_reset(void) {
    watchdog_test_ctx_t ctx;
    int i;
    ctx_init(&ctx);

    op_watchdog_config_t config = {0};
    config.transport = &ctx.transport;
    config.degrade_threshold = 1;
    config.recovery_threshold = 2;
    config.max_recovery_attempts = 5;
    config.backoff.base_interval_ms = 100u;
    config.backoff.max_interval_ms = 1000u;
    config.backoff.multiplier = 2.0f;
    config.state_cb = state_change_cb;
    config.state_cb_user_data = &ctx.tracker;
    ASSERT_EQ_INT(OP_STATUS_OK, op_watchdog_create(&config, &ctx.wd), "create watchdog");

    /* Trigger recovery and fail once to increase backoff */
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    op_watchdog_report_error(ctx.wd, OP_STATUS_IO_ERROR);
    ctx.mock.open_succeeds = 0;
    for (i = 0; i < 20; i++) {
        op_watchdog_tick(ctx.wd, 10u);
    }
    ASSERT_EQ_INT(200, op_watchdog_current_backoff_interval(ctx.wd), "backoff increased");

    /* Manually reset backoff */
    op_watchdog_reset_backoff(ctx.wd);
    ASSERT_EQ_INT(100, op_watchdog_current_backoff_interval(ctx.wd), "backoff reset manually");

    ctx_destroy(&ctx);
}

/* -- test: invalid backoff config ------------------------------------ */

static void test_backoff_invalid_config(void) {
    watchdog_test_ctx_t ctx;
    ctx_init(&ctx);
    ctx_create_std(&ctx, 2, 3);

    /* Invalid: base > max */
    op_watchdog_backoff_config_t invalid_config = {
        .base_interval_ms = 1000u,
        .max_interval_ms = 100u,
        .multiplier = 2.0f
    };
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_configure_backoff(ctx.wd, &invalid_config), "base > max rejected");

    /* Invalid: zero base */
    invalid_config.base_interval_ms = 0;
    invalid_config.max_interval_ms = 1000u;
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_configure_backoff(ctx.wd, &invalid_config), "zero base rejected");

    /* Invalid: zero multiplier */
    invalid_config.base_interval_ms = 100u;
    invalid_config.multiplier = 0.0f;
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_watchdog_configure_backoff(ctx.wd, &invalid_config), "zero multiplier rejected");

    ctx_destroy(&ctx);
}

int main(void) {
    printf("Running watchdog tests...\n");

    RUN_TEST(test_initial_state);
    RUN_TEST(test_error_degrade_transition);
    RUN_TEST(test_error_recovery_disconnected);
    RUN_TEST(test_recovery_succeeds);
    RUN_TEST(test_ok_resets_errors);
    RUN_TEST(test_degraded_recover_via_ok);
    RUN_TEST(test_force_reconnect);
    RUN_TEST(test_health_probe_failure);
    RUN_TEST(test_state_labels);
    RUN_TEST(test_null_safety);
    RUN_TEST(test_total_errors_persist);
    RUN_TEST(test_backoff_config);
    RUN_TEST(test_backoff_progression);
    RUN_TEST(test_backoff_reset_on_success);
    RUN_TEST(test_backoff_max_cap);
    RUN_TEST(test_backoff_runtime_config);
    RUN_TEST(test_backoff_manual_reset);
    RUN_TEST(test_backoff_invalid_config);

    if (test_failures != 0) {
        fprintf(stderr, "watchdog_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("watchdog_test: all tests passed\n");
    return 0;
}
