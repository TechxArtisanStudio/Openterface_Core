#include "openterface/transport.h"

void op_transport_init(op_transport_t *transport, op_transport_kind_t kind, void *context, const op_transport_vtable_t *vtable) {
    if (transport == NULL) {
        return;
    }

    transport->kind = kind;
    transport->context = context;
    transport->vtable = vtable;
    transport->is_open = 0;
}

op_status_t op_transport_open(op_transport_t *transport) {
    op_status_t status;

    if (transport == NULL || transport->vtable == NULL || transport->vtable->open == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = transport->vtable->open(transport->context);
    if (status == OP_STATUS_OK) {
        transport->is_open = 1;
    }
    return status;
}

op_status_t op_transport_close(op_transport_t *transport) {
    op_status_t status;

    if (transport == NULL || transport->vtable == NULL || transport->vtable->close == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = transport->vtable->close(transport->context);
    if (status == OP_STATUS_OK) {
        transport->is_open = 0;
    }
    return status;
}

op_status_t op_transport_read(op_transport_t *transport, uint8_t *buffer, size_t capacity, size_t *out_length) {
    if (transport == NULL || transport->vtable == NULL || transport->vtable->read == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return transport->vtable->read(transport->context, buffer, capacity, out_length);
}

op_status_t op_transport_write(op_transport_t *transport, const uint8_t *buffer, size_t length) {
    if (transport == NULL || transport->vtable == NULL || transport->vtable->write == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return transport->vtable->write(transport->context, buffer, length);
}

op_status_t op_transport_control(op_transport_t *transport, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    if (transport == NULL || transport->vtable == NULL || transport->vtable->control == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    return transport->vtable->control(transport->context, request, input, input_length, output, output_capacity, out_length);
}

const char *op_transport_kind_label(op_transport_kind_t kind) {
    switch (kind) {
        case OP_TRANSPORT_KIND_SERIAL:
            return "serial";
        case OP_TRANSPORT_KIND_HID_FEATURE:
            return "hid_feature";
        case OP_TRANSPORT_KIND_WEB_SERIAL:
            return "web_serial";
        case OP_TRANSPORT_KIND_WEB_HID:
            return "web_hid";
        case OP_TRANSPORT_KIND_LOCAL_SERVICE:
            return "local_service";
        case OP_TRANSPORT_KIND_BLE:
            return "ble";
        case OP_TRANSPORT_KIND_STUB:
            return "stub";
        default:
            return "unknown";
    }
}
