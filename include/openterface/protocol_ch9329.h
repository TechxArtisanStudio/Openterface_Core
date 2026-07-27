#ifndef OPENTERFACE_PROTOCOL_CH9329_H
#define OPENTERFACE_PROTOCOL_CH9329_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/input.h"
#include "openterface/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OP_CH9329_HEADER_0 0x57
#define OP_CH9329_HEADER_1 0xAB
#define OP_CH9329_ADDR_DEFAULT 0x00
#define OP_CH9329_PKT_KEYBOARD_SIZE OP_INPUT_PKT_KEYBOARD_SIZE
#define OP_CH9329_PKT_MOUSE_REL_SIZE OP_INPUT_PKT_MOUSE_REL_SIZE
#define OP_CH9329_PKT_MOUSE_ABS_SIZE OP_INPUT_PKT_MOUSE_ABS_SIZE
#define OP_CH9329_PKT_USB_SWITCH_SIZE 11u
#define OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE 7u
#define OP_CH9329_CMD_KEYBOARD OP_INPUT_CMD_KB
#define OP_CH9329_CMD_MOUSE_REL OP_INPUT_CMD_MS_REL
#define OP_CH9329_CMD_MOUSE_ABS OP_INPUT_CMD_MS_ABS
#define OP_CH9329_CMD_USB_SWITCH 0x17u
#define OP_CH9329_RESP_USB_SWITCH (OP_CH9329_CMD_USB_SWITCH | 0x80u)
#define OP_CH9329_USB_SWITCH_HOST 0x00u
#define OP_CH9329_USB_SWITCH_TARGET 0x01u
#define OP_CH9329_USB_SWITCH_QUERY 0x03u

uint8_t op_ch9329_checksum(const uint8_t data[], int len);
void op_ch9329_hex_dump(const uint8_t data[], int len, char out[]);
int op_ch9329_build_keyboard_packet(uint8_t out[OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, const uint8_t keys[], int num_keys);
int op_ch9329_build_mouse_rel_packet(uint8_t out[OP_CH9329_PKT_MOUSE_REL_SIZE], uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel);
int op_ch9329_build_mouse_abs_packet(uint8_t out[OP_CH9329_PKT_MOUSE_ABS_SIZE], uint8_t buttons, uint16_t x, uint16_t y, int8_t wheel);
int op_ch9329_build_press_release_packets(uint8_t out[2 * OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, uint8_t hid_code);
int op_ch9329_build_usb_switch_packet(uint8_t out[OP_CH9329_PKT_USB_SWITCH_SIZE], uint8_t request_type);
op_status_t op_ch9329_parse_usb_switch_response(const uint8_t *packet, size_t length, uint8_t *out_status);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_PROTOCOL_CH9329_H */
