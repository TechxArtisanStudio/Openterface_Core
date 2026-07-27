#include "openterface/core_native.h"

#include "platform_backend.h"

#include <stdlib.h>

typedef struct {
    const op_platform_serial_backend_t *backend;
    void *backend_context;
} op_native_serial_transport_context_t;

static op_status_t op_native_serial_transport_open(void *context) {
    op_native_serial_transport_context_t *serial = (op_native_serial_transport_context_t *)context;
    if (serial == NULL || serial->backend == NULL || serial->backend->open == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return serial->backend->open(serial->backend_context);
}

static op_status_t op_native_serial_transport_close(void *context) {
    op_native_serial_transport_context_t *serial = (op_native_serial_transport_context_t *)context;
    if (serial == NULL || serial->backend == NULL || serial->backend->close == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return serial->backend->close(serial->backend_context);
}

static op_status_t op_native_serial_transport_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    op_native_serial_transport_context_t *serial = (op_native_serial_transport_context_t *)context;
    if (serial == NULL || serial->backend == NULL || serial->backend->read == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return serial->backend->read(serial->backend_context, buffer, capacity, out_length);
}

static op_status_t op_native_serial_transport_write(void *context, const uint8_t *buffer, size_t length) {
    op_native_serial_transport_context_t *serial = (op_native_serial_transport_context_t *)context;
    if (serial == NULL || serial->backend == NULL || serial->backend->write == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return serial->backend->write(serial->backend_context, buffer, length);
}

static op_status_t op_native_serial_transport_control(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    (void)context;
    (void)request;
    (void)input;
    (void)input_length;
    (void)output;
    (void)output_capacity;
    (void)out_length;
    return OP_STATUS_NOT_SUPPORTED;
}

static const op_transport_vtable_t OP_NATIVE_SERIAL_TRANSPORT_VTABLE = {
    op_native_serial_transport_open,
    op_native_serial_transport_close,
    op_native_serial_transport_read,
    op_native_serial_transport_write,
    op_native_serial_transport_control,
};

op_version_t op_core_native_version(void) {
    op_version_t version = {0, 1, 0};
    return version;
}

const char *op_core_native_hid_backend_name(void) {
    return op_platform_get_hid_backend_name();
}

const char *op_core_native_serial_backend_name(const op_device_info_t *device) {
    return op_platform_get_serial_backend_name_for_device(device);
}

op_status_t op_native_transport_init_serial(op_transport_t *transport, const op_device_info_t *device) {
    const op_platform_serial_backend_t *backend;
    op_native_serial_transport_context_t *context;

    if (transport == NULL || device == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    backend = op_platform_get_serial_backend_for_device(device);
    if (backend == NULL || backend->create_context == NULL) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    context = (op_native_serial_transport_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    context->backend = backend;
    context->backend_context = backend->create_context(device);
    if (context->backend_context == NULL) {
        free(context);
        return OP_STATUS_IO_ERROR;
    }

    op_transport_init(transport, OP_TRANSPORT_KIND_SERIAL, context, &OP_NATIVE_SERIAL_TRANSPORT_VTABLE);
    return OP_STATUS_OK;
}

void op_native_transport_release(op_transport_t *transport) {
    op_native_serial_transport_context_t *context;

    if (transport == NULL || transport->context == NULL) {
        return;
    }

    if (transport->vtable != &OP_NATIVE_SERIAL_TRANSPORT_VTABLE) {
        return;
    }

    context = (op_native_serial_transport_context_t *)transport->context;
    if (transport->is_open && context->backend != NULL && context->backend->close != NULL) {
        (void)context->backend->close(context->backend_context);
    }
    if (context->backend != NULL && context->backend->destroy_context != NULL) {
        context->backend->destroy_context(context->backend_context);
    }

    free(context);
    transport->kind = OP_TRANSPORT_KIND_UNKNOWN;
    transport->context = NULL;
    transport->vtable = NULL;
    transport->is_open = 0;
}
