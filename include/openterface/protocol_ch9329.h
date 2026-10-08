#ifndef OPENTERFACE_PROTOCOL_CH9329_H
#define OPENTERFACE_PROTOCOL_CH9329_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
#define OP_CH9329_RESP_KEYBOARD (OP_CH9329_CMD_KEYBOARD | 0x80u)
#define OP_CH9329_RESP_MOUSE_REL (OP_CH9329_CMD_MOUSE_REL | 0x80u)
#define OP_CH9329_RESP_MOUSE_ABS (OP_CH9329_CMD_MOUSE_ABS | 0x80u)
#define OP_CH9329_USB_SWITCH_HOST 0x00u
#define OP_CH9329_USB_SWITCH_TARGET 0x01u
#define OP_CH9329_USB_SWITCH_QUERY 0x03u

uint8_t op_ch9329_checksum(const uint8_t data[], int len);
void op_ch9329_hex_dump(const uint8_t data[], int len, char out[]);
int op_ch9329_build_keyboard_packet(uint8_t out[OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, const uint8_t keys[], int num_keys, uint8_t flags);
int op_ch9329_build_mouse_rel_packet(uint8_t out[OP_CH9329_PKT_MOUSE_REL_SIZE], uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel);
int op_ch9329_build_mouse_abs_packet(uint8_t out[OP_CH9329_PKT_MOUSE_ABS_SIZE], uint8_t buttons, uint16_t x, uint16_t y, int8_t wheel);
int op_ch9329_build_press_release_packets(uint8_t out[2 * OP_CH9329_PKT_KEYBOARD_SIZE], uint8_t modifiers, uint8_t hid_code, uint8_t flags);
int op_ch9329_build_usb_switch_packet(uint8_t out[OP_CH9329_PKT_USB_SWITCH_SIZE], uint8_t request_type);
op_status_t op_ch9329_parse_usb_switch_response(const uint8_t *packet, size_t length, uint8_t *out_status);

/* ── Parsed packet ────────────────────────────────────────────────────── */

/** Maximum payload length (1-byte length field) */
#define OP_CH9329_MAX_PAYLOAD_SIZE 255u

/** Minimum packet size: 5-byte header + 1-byte checksum = 6 bytes */
#define OP_CH9329_MIN_PACKET_SIZE 6u

/** CH9329 packet parse result.
 *
 * Represents a fully decoded CH9329 packet:
 *   header[0..4] = 0x57 0xAB addr cmd len
 *   data[0..data_len-1] = payload
 *   checksum = received checksum (last byte)
 *   checksum_calc = computed checksum
 *   valid = true when checksum matches
 */
typedef struct {
    uint8_t header[5];
    uint8_t data[OP_CH9329_MAX_PAYLOAD_SIZE];
    uint8_t data_len;
    uint8_t checksum;
    uint8_t checksum_calc;
    bool valid;
} op_ch9329_parsed_packet_t;

/** Parse a raw byte stream into a structured CH9329 packet.
 *
 * @param raw   Input byte buffer (complete packet including header + checksum)
 * @param len   Number of bytes in @p raw
 * @param out   Output parsed packet (filled on success or checksum mismatch)
 * @return OP_STATUS_OK on success
 * @return OP_STATUS_INVALID_ARGUMENT if raw or out is NULL
 * @return OP_STATUS_IO_ERROR if the packet is malformed or checksum is invalid
 */
op_status_t op_ch9329_parse_packet(const uint8_t *raw, int len,
                                    op_ch9329_parsed_packet_t *out);

/* ── Specific response parsers ──────────────────────────────────────────── */

/** Keyboard response (cmd = 0x82).
 * Echoes back the keyboard packet data: modifiers + reserved + 6 key slots. */
typedef struct {
    uint8_t modifiers;     /* Modifier bitmask */
    uint8_t reserved;      /* Reserved byte (always 0) */
    uint8_t keys[6];       /* 6 key slots (HID codes, 0 = empty) */
} op_ch9329_keyboard_response_t;

/** Parse a keyboard response packet.
 * @param packet   Raw packet bytes (14 bytes expected)
 * @param length   Number of bytes in packet
 * @param out      Output parsed response
 * @return OP_STATUS_OK on success
 * @return OP_STATUS_INVALID_ARGUMENT if packet or out is NULL
 * @return OP_STATUS_IO_ERROR if packet is malformed or checksum invalid
 */
op_status_t op_ch9329_parse_keyboard_response(const uint8_t *packet, size_t length,
                                               op_ch9329_keyboard_response_t *out);

/** Mouse relative response (cmd = 0x85).
 * Echoes back mouse relative data: mode + buttons + dx + dy + wheel. */
typedef struct {
    uint8_t mode;          /* 0x01 = relative mode */
    uint8_t buttons;       /* Button bitmask */
    int8_t dx;             /* X movement */
    int8_t dy;             /* Y movement */
    int8_t wheel;          /* Scroll wheel */
} op_ch9329_mouse_rel_response_t;

/** Parse a mouse relative response packet.
 * @param packet   Raw packet bytes (11 bytes expected)
 * @param length   Number of bytes in packet
 * @param out      Output parsed response
 * @return OP_STATUS_OK on success
 * @return OP_STATUS_INVALID_ARGUMENT if packet or out is NULL
 * @return OP_STATUS_IO_ERROR if packet is malformed or checksum invalid
 */
op_status_t op_ch9329_parse_mouse_rel_response(const uint8_t *packet, size_t length,
                                                op_ch9329_mouse_rel_response_t *out);

/** Mouse absolute response (cmd = 0x84).
 * Echoes back mouse absolute data: mode + buttons + x + y + wheel. */
typedef struct {
    uint8_t mode;          /* 0x02 = absolute mode */
    uint8_t buttons;       /* Button bitmask */
    uint16_t x;            /* X coordinate (0-4096) */
    uint16_t y;            /* Y coordinate (0-4096) */
    int8_t wheel;          /* Scroll wheel */
} op_ch9329_mouse_abs_response_t;

/** Parse a mouse absolute response packet.
 * @param packet   Raw packet bytes (13 bytes expected)
 * @param length   Number of bytes in packet
 * @param out      Output parsed response
 * @return OP_STATUS_OK on success
 * @return OP_STATUS_INVALID_ARGUMENT if packet or out is NULL
 * @return OP_STATUS_IO_ERROR if packet is malformed or checksum invalid
 */
op_status_t op_ch9329_parse_mouse_abs_response(const uint8_t *packet, size_t length,
                                                op_ch9329_mouse_abs_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_PROTOCOL_CH9329_H */
