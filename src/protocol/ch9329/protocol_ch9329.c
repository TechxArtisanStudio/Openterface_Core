#include "openterface/protocol_ch9329.h"

#include <string.h>

uint8_t op_ch9329_checksum(const uint8_t data[], int len) {
    return op_input_checksum(data, len);
}

void op_ch9329_hex_dump(const uint8_t data[], int len, char out[]) {
    op_input_hex_dump(data, len, out);
}

int op_ch9329_build_keyboard_packet(uint8_t out[OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, const uint8_t keys[], int num_keys) {
    return op_input_build_keyboard(out, modifiers, keys, num_keys);
}

int op_ch9329_build_mouse_rel_packet(uint8_t out[OP_CH9329_PKT_MOUSE_REL_SIZE], uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel) {
    return op_input_build_mouse_rel(out, buttons, dx, dy, wheel);
}

int op_ch9329_build_mouse_abs_packet(uint8_t out[OP_CH9329_PKT_MOUSE_ABS_SIZE], uint8_t buttons, uint16_t x, uint16_t y, int8_t wheel) {
    return op_input_build_mouse_abs(out, buttons, x, y, wheel);
}

int op_ch9329_build_press_release_packets(uint8_t out[2 * OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, uint8_t hid_code) {
    return op_input_build_press_release(out, modifiers, hid_code);
}

int op_ch9329_build_usb_switch_packet(uint8_t out[OP_CH9329_PKT_USB_SWITCH_SIZE], uint8_t request_type) {
    if (out == NULL) {
        return 0;
    }

    memset(out, 0, OP_CH9329_PKT_USB_SWITCH_SIZE);
    out[0] = OP_CH9329_HEADER_0;
    out[1] = OP_CH9329_HEADER_1;
    out[2] = OP_CH9329_ADDR_DEFAULT;
    out[3] = OP_CH9329_CMD_USB_SWITCH;
    out[4] = 0x05u;
    out[9] = request_type;
    out[10] = op_ch9329_checksum(out, OP_CH9329_PKT_USB_SWITCH_SIZE);
    return (int)OP_CH9329_PKT_USB_SWITCH_SIZE;
}

op_status_t op_ch9329_parse_usb_switch_response(const uint8_t *packet, size_t length, uint8_t *out_status) {
    if (packet == NULL || out_status == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (length < OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE) {
        return OP_STATUS_IO_ERROR;
    }

    if (packet[0] != OP_CH9329_HEADER_0 || packet[1] != OP_CH9329_HEADER_1 || packet[2] != OP_CH9329_ADDR_DEFAULT) {
        return OP_STATUS_IO_ERROR;
    }

    if (packet[3] != OP_CH9329_RESP_USB_SWITCH || packet[4] != 0x01u) {
        return OP_STATUS_IO_ERROR;
    }

    if (op_ch9329_checksum(packet, (int)OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE) != packet[OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE - 1u]) {
        return OP_STATUS_IO_ERROR;
    }

    if (packet[5] != OP_CH9329_USB_SWITCH_HOST && packet[5] != OP_CH9329_USB_SWITCH_TARGET) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    *out_status = packet[5];
    return OP_STATUS_OK;
}
