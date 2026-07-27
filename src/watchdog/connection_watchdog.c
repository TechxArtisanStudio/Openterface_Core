#include "openterface/watchdog.h"

#include <stdlib.h>
#include <string.h>

/* ── defaults ─────────────────────────────────────────────────────── */

#define OP_WD_DEGRADE_THRESHOLD_DEFAULT   3u
#define OP_WD_RECOVERY_THRESHOLD_DEFAULT  5u
#define OP_WD_MAX_RECOVERY_ATTEMPTS_DEFAULT 5u
#define OP_WD_HEALTH_CHECK_INTERVAL_MS    5000u
#define OP_WD_RECOVERY_DELAY_MS           1000u

/* ── internal recovery sub-states ─────────────────────────────────── */

typedef enum {
    OP_WD_RECOV_NONE,     /* not in recovery */
    OP_WD_RECOV_CLOSING,  /* close the transport */
    OP_WD_RECOV_WAITING,  /* wait recovery_delay_ms before reopen */
    OP_WD_RECOV_OPENING,  /* attempt reopen */
} op_watchdog_recovery_phase_t;

/* ── watchdog struct ──────────────────────────────────────────────── */

struct op_watchdog_t {
    op_transport_t *transport;

    /* thresholds */
    uint32_t degrade_threshold;
    uint32_t recovery_threshold;
    uint32_t max_recovery_attempts;
    uint32_t health_check_interval_ms;
    uint32_t recovery_delay_ms;

    /* state */
    op_connection_state_t state;
    uint32_t consecutive_errors;
    uint32_t total_errors;
    uint32_t recovery_attempts;
    op_watchdog_recovery_phase_t recovery_phase;

    /* timing */
    uint32_t last_health_check_ms;     /* ms since watchdog_create */
    uint32_t recovery_timer_ms;        /* counts up during RECOV_WAITING */

    /* health probe */
    op_health_probe_fn_t health_probe;
    void *health_probe_context;

    /* callback */
    op_connection_state_cb_t state_cb;
    void *state_cb_user_data;
};

/* ── helpers ──────────────────────────────────────────────────────── */

static void op_watchdog_set_state(op_watchdog_t *wd, op_connection_state_t new_state, op_status_t error) {
    if (wd == NULL) {
        return;
    }

    op_connection_state_t old_state = wd->state;
    if (old_state == new_state) {
        return;
    }

    wd->state = new_state;

    if (wd->state_cb != NULL) {
        op_connection_event_t event;
        event.old_state = old_state;
        event.new_state = new_state;
        event.last_error = error;
        event.consecutive_errors = wd->consecutive_errors;
        event.user_data = wd->state_cb_user_data;
        wd->state_cb(&event);
    }
}

static void op_watchdog_reset_to_connected(op_watchdog_t *wd) {
    wd->consecutive_errors = 0;
    wd->recovery_attempts = 0;
    wd->recovery_phase = OP_WD_RECOV_NONE;
    wd->last_health_check_ms = 0;
    op_watchdog_set_state(wd, OP_CONN_STATE_CONNECTED, OP_STATUS_OK);
}

static op_status_t op_watchdog_run_health_probe(op_watchdog_t *wd) {
    if (wd->health_probe == NULL) {
        return OP_STATUS_OK;
    }

    return wd->health_probe(wd->health_probe_context);
}

/* ── public API ───────────────────────────────────────────────────── */

op_status_t op_watchdog_create(const op_watchdog_config_t *config, op_watchdog_t **out_watchdog) {
    op_watchdog_t *wd;

    if (config == NULL || config->transport == NULL || out_watchdog == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    wd = (op_watchdog_t *)calloc(1, sizeof(*wd));
    if (wd == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    wd->transport = config->transport;
    wd->degrade_threshold = config->degrade_threshold != 0u ? config->degrade_threshold : OP_WD_DEGRADE_THRESHOLD_DEFAULT;
    wd->recovery_threshold = config->recovery_threshold != 0u ? config->recovery_threshold : OP_WD_RECOVERY_THRESHOLD_DEFAULT;
    wd->max_recovery_attempts = config->max_recovery_attempts != 0u ? config->max_recovery_attempts : OP_WD_MAX_RECOVERY_ATTEMPTS_DEFAULT;
    wd->health_check_interval_ms = config->health_check_interval_ms != 0u ? config->health_check_interval_ms : OP_WD_HEALTH_CHECK_INTERVAL_MS;
    wd->recovery_delay_ms = config->recovery_delay_ms != 0u ? config->recovery_delay_ms : OP_WD_RECOVERY_DELAY_MS;
    wd->state = OP_CONN_STATE_CONNECTED;
    wd->recovery_phase = OP_WD_RECOV_NONE;
    wd->health_probe = config->health_probe;
    wd->health_probe_context = config->health_probe_context;
    wd->state_cb = config->state_cb;
    wd->state_cb_user_data = config->state_cb_user_data;

    *out_watchdog = wd;
    return OP_STATUS_OK;
}

void op_watchdog_destroy(op_watchdog_t *watchdog) {
    free(watchdog);
}

void op_watchdog_tick(op_watchdog_t *wd, uint32_t elapsed_ms) {
    if (wd == NULL) {
        return;
    }

    /* advance health check timer */
    wd->last_health_check_ms += elapsed_ms;

    switch (wd->state) {
        case OP_CONN_STATE_CONNECTED:
        case OP_CONN_STATE_DEGRADED:
            /* periodic health probe */
            if (wd->last_health_check_ms >= wd->health_check_interval_ms) {
                wd->last_health_check_ms = 0;
                op_status_t status = op_watchdog_run_health_probe(wd);
                if (status != OP_STATUS_OK) {
                    op_watchdog_report_error(wd, status);
                }
            }
            break;

        case OP_CONN_STATE_RECOVERING:
            switch (wd->recovery_phase) {
                case OP_WD_RECOV_CLOSING:
                    if (wd->transport != NULL && wd->transport->is_open) {
                        (void)op_transport_close(wd->transport);
                    }
                    wd->recovery_phase = OP_WD_RECOV_WAITING;
                    wd->recovery_timer_ms = 0;
                    break;

                case OP_WD_RECOV_WAITING:
                    wd->recovery_timer_ms += elapsed_ms;
                    if (wd->recovery_timer_ms >= wd->recovery_delay_ms) {
                        wd->recovery_phase = OP_WD_RECOV_OPENING;
                    }
                    break;

                case OP_WD_RECOV_OPENING:
                    if (wd->transport != NULL) {
                        op_status_t status = op_transport_open(wd->transport);
                        if (status == OP_STATUS_OK) {
                            /* reopen succeeded, verify with health probe */
                            status = op_watchdog_run_health_probe(wd);
                        }
                        if (status == OP_STATUS_OK && wd->recovery_attempts < wd->max_recovery_attempts) {
                            op_watchdog_reset_to_connected(wd);
                        } else {
                            wd->recovery_attempts++;
                            wd->state = OP_CONN_STATE_DISCONNECTED;
                            wd->recovery_phase = OP_WD_RECOV_NONE;
                            if (wd->recovery_attempts < wd->max_recovery_attempts) {
                                wd->state = OP_CONN_STATE_RECOVERING;
                                wd->recovery_phase = OP_WD_RECOV_WAITING;
                                wd->recovery_timer_ms = 0;
                            }
                        }
                    } else {
                        wd->recovery_attempts++;
                        if (wd->recovery_attempts >= wd->max_recovery_attempts) {
                            wd->state = OP_CONN_STATE_DISCONNECTED;
                            wd->recovery_phase = OP_WD_RECOV_NONE;
                        } else {
                            wd->recovery_phase = OP_WD_RECOV_WAITING;
                            wd->recovery_timer_ms = 0;
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case OP_CONN_STATE_DISCONNECTED:
            /* idle — waiting for force_reconnect or app intervention */
            break;
    }
}

void op_watchdog_report_error(op_watchdog_t *wd, op_status_t error) {
    if (wd == NULL) {
        return;
    }

    wd->consecutive_errors++;
    wd->total_errors++;

    switch (wd->state) {
        case OP_CONN_STATE_CONNECTED:
            if (wd->consecutive_errors >= wd->recovery_threshold) {
                wd->recovery_phase = OP_WD_RECOV_CLOSING;
                wd->recovery_attempts = 0;
                op_watchdog_set_state(wd, OP_CONN_STATE_RECOVERING, error);
            } else if (wd->consecutive_errors >= wd->degrade_threshold) {
                op_watchdog_set_state(wd, OP_CONN_STATE_DEGRADED, error);
            }
            break;

        case OP_CONN_STATE_DEGRADED:
            if (wd->consecutive_errors >= wd->recovery_threshold) {
                wd->recovery_phase = OP_WD_RECOV_CLOSING;
                wd->recovery_attempts = 0;
                op_watchdog_set_state(wd, OP_CONN_STATE_RECOVERING, error);
            }
            break;

        case OP_CONN_STATE_RECOVERING:
        case OP_CONN_STATE_DISCONNECTED:
            /* errors during recovery/disconnected are expected, no state change */
            break;
    }
}

void op_watchdog_report_ok(op_watchdog_t *wd) {
    if (wd == NULL) {
        return;
    }

    wd->consecutive_errors = 0;
    wd->last_health_check_ms = 0;

    if (wd->state == OP_CONN_STATE_DEGRADED) {
        /* A success while degraded means the link is functional again.
           Transition back to connected. */
        op_watchdog_set_state(wd, OP_CONN_STATE_CONNECTED, OP_STATUS_OK);
    }
}

op_connection_state_t op_watchdog_get_state(const op_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return OP_CONN_STATE_DISCONNECTED;
    }

    return watchdog->state;
}

uint32_t op_watchdog_consecutive_errors(const op_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return 0;
    }

    return watchdog->consecutive_errors;
}

uint32_t op_watchdog_total_errors(const op_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return 0;
    }

    return watchdog->total_errors;
}

uint32_t op_watchdog_recovery_attempts(const op_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return 0;
    }

    return watchdog->recovery_attempts;
}

op_status_t op_watchdog_force_reconnect(op_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    watchdog->consecutive_errors = 0;
    watchdog->recovery_attempts = 0;
    watchdog->recovery_phase = OP_WD_RECOV_CLOSING;
    watchdog->recovery_timer_ms = 0;

    if (watchdog->state != OP_CONN_STATE_RECOVERING) {
        op_watchdog_set_state(watchdog, OP_CONN_STATE_RECOVERING, OP_STATUS_OK);
    }

    return OP_STATUS_OK;
}

const char *op_connection_state_label(op_connection_state_t state) {
    switch (state) {
        case OP_CONN_STATE_CONNECTED:   return "connected";
        case OP_CONN_STATE_DEGRADED:    return "degraded";
        case OP_CONN_STATE_RECOVERING:  return "recovering";
        case OP_CONN_STATE_DISCONNECTED: return "disconnected";
        default:                        return "unknown";
    }
}
