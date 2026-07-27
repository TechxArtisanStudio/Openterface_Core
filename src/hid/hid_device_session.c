#include "hid_internal.h"

#include <stdlib.h>
#include <string.h>

static int op_hid_device_should_use_stub_backend(const op_device_info_t *device) {
    static const char prefix[] = "stub:";

    if (device == NULL) {
        return 0;
    }

    return strncmp(device->hid_path, prefix, sizeof(prefix) - 1u) == 0;
}

op_status_t op_hid_device_session_create(const op_device_info_t *device, op_hid_device_session_t **out_session) {
    op_hid_device_session_t *session;

    if (device == NULL || out_session == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    session = (op_hid_device_session_t *)calloc(1, sizeof(*session));
    if (session == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    memcpy(&session->device, device, sizeof(*device));
    session->backend = op_hid_device_should_use_stub_backend(device)
        ? op_platform_get_stub_hid_backend()
        : op_platform_get_hid_backend();
    if (session->backend == NULL || session->backend->create_context == NULL) {
        free(session);
        return OP_STATUS_UNINITIALIZED;
    }

    session->backend_context = session->backend->create_context(device);
    if (session->backend_context == NULL) {
        free(session);
        return OP_STATUS_IO_ERROR;
    }

    *out_session = session;
    return OP_STATUS_OK;
}

void op_hid_device_session_destroy(op_hid_device_session_t *session) {
    if (session == NULL) {
        return;
    }

    if (session->backend != NULL && session->backend->close != NULL && session->is_open) {
        session->backend->close(session->backend_context);
    }

    if (session->backend != NULL && session->backend->destroy_context != NULL) {
        session->backend->destroy_context(session->backend_context);
    }

    free(session);
}

op_status_t op_hid_device_session_open(op_hid_device_session_t *session) {
    op_status_t status;

    if (session == NULL || session->backend == NULL || session->backend->open == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = session->backend->open(session->backend_context);
    if (status == OP_STATUS_OK) {
        session->is_open = 1;
    }

    return status;
}

op_status_t op_hid_device_session_close(op_hid_device_session_t *session) {
    op_status_t status;

    if (session == NULL || session->backend == NULL || session->backend->close == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = session->backend->close(session->backend_context);
    if (status == OP_STATUS_OK) {
        session->is_open = 0;
        session->transaction_active = 0;
    }

    return status;
}

int op_hid_device_session_is_open(const op_hid_device_session_t *session) {
    if (session == NULL) {
        return 0;
    }

    return session->is_open;
}

const op_device_info_t *op_hid_device_session_device(const op_hid_device_session_t *session) {
    if (session == NULL) {
        return NULL;
    }

    return &session->device;
}

op_status_t op_hid_device_session_send_feature_report(op_hid_device_session_t *session, const uint8_t *report, size_t length) {
    if (session == NULL || session->backend == NULL || session->backend->send_feature_report == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return session->backend->send_feature_report(session->backend_context, report, length);
}

op_status_t op_hid_device_session_get_feature_report(op_hid_device_session_t *session, uint8_t *report, size_t capacity, size_t *out_length) {
    if (session == NULL || session->backend == NULL || session->backend->get_feature_report == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return session->backend->get_feature_report(session->backend_context, report, capacity, out_length);
}
