#ifndef OPENTERFACE_WATCHDOG_H
#define OPENTERFACE_WATCHDOG_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/native_common.h"
#include "openterface/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Connection state machine ─────────────────────────────────────── */

typedef enum {
    OP_CONN_STATE_CONNECTED = 0,       /* healthy, no errors */
    OP_CONN_STATE_DEGRADED = 1,        /* sporadic errors, still functional */
    OP_CONN_STATE_RECOVERING = 2,      /* close→reopen in progress */
    OP_CONN_STATE_DISCONNECTED = 3,    /* recovery exhausted or device gone */
} op_connection_state_t;

/* Event passed to the state-change callback */
typedef struct {
    op_connection_state_t old_state;
    op_connection_state_t new_state;
    op_status_t last_error;             /* status that triggered the transition */
    uint32_t consecutive_errors;        /* error count at time of transition */
    void *user_data;                    /* opaque pointer from watchdog config */
} op_connection_event_t;

typedef void (*op_connection_state_cb_t)(const op_connection_event_t *event);

/* ── Health probe ─────────────────────────────────────────────────── */

/* Optional probe the caller provides so the watchdog can verify the
   device is alive (e.g. read a known register, send a serial echo).
   The watchdog calls this before declaring a connection "healthy".
   Return OP_STATUS_OK if the device responded, any other status if not. */
typedef op_status_t (*op_health_probe_fn_t)(void *context);

/* ── Configuration ────────────────────────────────────────────────── */

typedef struct {
    op_transport_t *transport;          /* transport to monitor */

    /* Error thresholds */
    uint32_t degrade_threshold;         /* consecutive errors before DEGRADED (default: 3) */
    uint32_t recovery_threshold;        /* consecutive errors before RECOVERING (default: 5) */
    uint32_t max_recovery_attempts;     /* max close→reopen cycles before DISCONNECTED (default: 5) */

    /* Timing (milliseconds) */
    uint32_t health_check_interval_ms;  /* interval between health probes (default: 5000) */
    uint32_t recovery_delay_ms;         /* wait between close and reopen during recovery (default: 1000) */

    /* Callbacks */
    op_connection_state_cb_t state_cb;  /* called on every state transition */
    void *state_cb_user_data;           /* passed to state_cb */

    /* Health probe (optional — if NULL, watchdog only tracks transport errors) */
    op_health_probe_fn_t health_probe;
    void *health_probe_context;
} op_watchdog_config_t;

/* ── Watchdog handle ──────────────────────────────────────────────── */

typedef struct op_watchdog_t op_watchdog_t;

op_status_t op_watchdog_create(const op_watchdog_config_t *config, op_watchdog_t **out_watchdog);
void op_watchdog_destroy(op_watchdog_t *watchdog);

/* Call periodically (suggested: every 100–500 ms). This drives:
   - error window decay
   - health probe scheduling
   - recovery timing
   All timing is tick-driven — no internal threads. */
void op_watchdog_tick(op_watchdog_t *watchdog, uint32_t elapsed_ms);

/* Report a transport-layer error to the watchdog.
   Call this whenever any operation on the monitored transport fails.
   The watchdog decides whether to degrade, recover, or disconnect. */
void op_watchdog_report_error(op_watchdog_t *watchdog, op_status_t error);

/* Report a successful operation. Resets the consecutive error counter
   and may transition back to CONNECTED if recovering. */
void op_watchdog_report_ok(op_watchdog_t *watchdog);

/* Get current connection state */
op_connection_state_t op_watchdog_get_state(const op_watchdog_t *watchdog);

/* Get consecutive error count */
uint32_t op_watchdog_consecutive_errors(const op_watchdog_t *watchdog);

/* Get total error count since creation */
uint32_t op_watchdog_total_errors(const op_watchdog_t *watchdog);

/* Get recovery attempt count for current disconnection episode */
uint32_t op_watchdog_recovery_attempts(const op_watchdog_t *watchdog);

/* Force a reconnection attempt regardless of current state.
   Useful when the app detects the device came back (e.g. hotplug event). */
op_status_t op_watchdog_force_reconnect(op_watchdog_t *watchdog);

const char *op_connection_state_label(op_connection_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_WATCHDOG_H */
