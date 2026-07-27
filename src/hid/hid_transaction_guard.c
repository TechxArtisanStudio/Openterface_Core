#include "hid_internal.h"

#include <stdlib.h>

op_status_t op_hid_transaction_guard_create(op_hid_device_session_t *session, const op_hid_transaction_options_t *options, op_hid_transaction_guard_t **out_guard) {
    op_hid_transaction_guard_t *guard;

    if (session == NULL || out_guard == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    guard = (op_hid_transaction_guard_t *)calloc(1, sizeof(*guard));
    if (guard == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    guard->session = session;
    if (options != NULL) {
        guard->options = *options;
    }

    *out_guard = guard;
    return OP_STATUS_OK;
}

void op_hid_transaction_guard_destroy(op_hid_transaction_guard_t *guard) {
    if (guard == NULL) {
        return;
    }

    if (guard->entered) {
        op_hid_transaction_guard_leave(guard);
    }

    free(guard);
}

op_status_t op_hid_transaction_guard_enter(op_hid_transaction_guard_t *guard) {
    if (guard == NULL || guard->session == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!guard->session->is_open) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (guard->session->transaction_active) {
        return OP_STATUS_BUSY;
    }

    guard->session->transaction_active = 1;
    guard->entered = 1;
    return OP_STATUS_OK;
}

void op_hid_transaction_guard_leave(op_hid_transaction_guard_t *guard) {
    if (guard == NULL || guard->session == NULL || !guard->entered) {
        return;
    }

    guard->session->transaction_active = 0;
    guard->entered = 0;
}
