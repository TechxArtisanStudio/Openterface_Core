#include "usb_mode_internal.h"

#include "openterface/protocol_ch9329.h"
#include "openterface/profile.h"

static const op_device_info_t *op_usb_mode_serial_device(const op_usb_mode_endpoint_t *endpoint) {
    if (endpoint == NULL) {
        return NULL;
    }

    if (endpoint->device != NULL) {
        return endpoint->device;
    }

    if (endpoint->hid_session != NULL) {
        return op_hid_device_session_device(endpoint->hid_session);
    }

    if (endpoint->chip_controller != NULL && endpoint->chip_controller->session != NULL) {
        return op_hid_device_session_device(endpoint->chip_controller->session);
    }

    return NULL;
}

static int op_usb_mode_serial_matches(const op_usb_mode_endpoint_t *endpoint) {
    const op_device_info_t *device = op_usb_mode_serial_device(endpoint);

    if (device == NULL) {
        return 0;
    }

    return (device->protocol_flags & (uint32_t)OP_PROTOCOL_CH32V208) != 0u;
}

static op_status_t op_usb_mode_serial_query(op_usb_mode_endpoint_t *endpoint, uint8_t request_type, op_usb_mode_t *out_mode) {
    uint8_t request[OP_CH9329_PKT_USB_SWITCH_SIZE];
    uint8_t response[OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE];
    size_t response_length = 0u;
    uint8_t status = 0u;
    op_status_t transport_status;

    if (endpoint == NULL || out_mode == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (endpoint->transport == NULL) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return OP_STATUS_UNINITIALIZED;
    }

    if (!endpoint->transport->is_open) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return OP_STATUS_UNINITIALIZED;
    }

    if (op_ch9329_build_usb_switch_packet(request, request_type) != (int)OP_CH9329_PKT_USB_SWITCH_SIZE) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return OP_STATUS_IO_ERROR;
    }

    transport_status = op_transport_write(endpoint->transport, request, OP_CH9329_PKT_USB_SWITCH_SIZE);
    if (transport_status != OP_STATUS_OK) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return transport_status;
    }

    transport_status = op_transport_read(endpoint->transport, response, sizeof(response), &response_length);
    if (transport_status != OP_STATUS_OK) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return transport_status;
    }

    transport_status = op_ch9329_parse_usb_switch_response(response, response_length, &status);
    if (transport_status != OP_STATUS_OK) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return transport_status;
    }

    *out_mode = status == OP_CH9329_USB_SWITCH_TARGET ? OP_USB_MODE_TARGET : OP_USB_MODE_HOST;
    return OP_STATUS_OK;
}

static op_status_t op_usb_mode_serial_get_mode(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t *out_mode) {
    return op_usb_mode_serial_query(endpoint, OP_CH9329_USB_SWITCH_QUERY, out_mode);
}

static op_status_t op_usb_mode_serial_set_mode(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t mode) {
    op_usb_mode_t observed_mode = OP_USB_MODE_UNKNOWN;
    uint8_t request_type;
    op_status_t status;

    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (mode != OP_USB_MODE_HOST && mode != OP_USB_MODE_TARGET) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    request_type = mode == OP_USB_MODE_TARGET ? OP_CH9329_USB_SWITCH_TARGET : OP_CH9329_USB_SWITCH_HOST;
    status = op_usb_mode_serial_query(endpoint, request_type, &observed_mode);
    if (status != OP_STATUS_OK) {
        return status;
    }

    return observed_mode == mode ? OP_STATUS_OK : OP_STATUS_IO_ERROR;
}

const op_usb_mode_provider_t *op_usb_mode_serial_provider(void) {
    static const op_usb_mode_provider_t provider = {
        op_usb_mode_serial_matches,
        op_usb_mode_serial_get_mode,
        op_usb_mode_serial_set_mode
    };

    return &provider;
}