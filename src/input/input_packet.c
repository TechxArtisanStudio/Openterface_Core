#include "openterface/input.h"

#include <string.h>

/* Common 5-byte magic header: 57 AB 00 <cmd> <len> */
static const uint8_t KB_HDR[]     = { 0x57, 0xAB, 0x00, OP_INPUT_CMD_KB,     0x08 };
static const uint8_t MS_HDR[]     = { 0x57, 0xAB, 0x00, OP_INPUT_CMD_MS_REL, 0x05 };
static const uint8_t MS_ABS_HDR[] = { 0x57, 0xAB, 0x00, OP_INPUT_CMD_MS_ABS, 0x07 };

/* Mouse mode byte (byte 5 after 5-byte header) */
enum {
    OP_INPUT_MS_MODE_REL = 0x01,
    OP_INPUT_MS_MODE_ABS = 0x02,
};

static void copy_packet_header(uint8_t *out, const uint8_t header[5]) {
    memcpy(out, header, 5);
}

static void finalize_packet(uint8_t *out, int len) {
    out[len - 1] = op_input_checksum(out, len);
}

static int clamped_key_count(int num_keys) {
    if (num_keys <= 0) return 0;
    return num_keys < 6 ? num_keys : 6;
}

static void write_key_slots(uint8_t *out, const uint8_t keys[], int num_keys) {
    int count = clamped_key_count(num_keys);
    for (int i = 0; i < count; i++) {
        out[7 + i] = keys[i];
    }
    for (int i = count; i < 6; i++) {
        out[7 + i] = 0x00;
    }
}

uint8_t op_input_checksum(const uint8_t data[], int len) {
    uint32_t sum = 0;
    for (int i = 0; i < len - 1; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

void op_input_hex_dump(const uint8_t data[], int len, char out[]) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < len; i++) {
        out[i * 2]     = hex[(data[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

/* HID codes for modifier keys */
static const uint8_t MOD_HID_CODES[8] = {
    0xE0, /* LCtrl  bit 0 */
    0xE1, /* LShift bit 1 */
    0xE2, /* LAlt   bit 2 */
    0xE3, /* LGUI   bit 3 */
    0xE4, /* RCtrl  bit 4 */
    0xE5, /* RShift bit 5 */
    0xE6, /* RAlt   bit 6 */
    0xE7, /* RGUI   bit 7 */
};

int op_input_build_keyboard(uint8_t out[OP_INPUT_PKT_KEYBOARD_SIZE],
                             uint8_t modifiers,
                             const uint8_t keys[],
                             int num_keys,
                             uint8_t flags) {
    copy_packet_header(out, KB_HDR);

    if (flags & OP_INPUT_KB_FLAG_CH9329_WORKAROUND) {
        /* Workaround: keep Ctrl/Shift in modifier byte (left and right) */
        out[5] = modifiers & OP_INPUT_MOD_CTRL_SHIFT_MASK;
        out[6] = 0x00;

        /* Expand all modifiers to HID codes in key array */
        int idx = 0;
        for (int bit = 0; bit < 8 && idx < 6; bit++) {
            if (modifiers & (1 << bit)) {
                out[7 + idx++] = MOD_HID_CODES[bit];
            }
        }

        /* Append normal keys */
        int key_count = clamped_key_count(num_keys);
        int remaining_slots = 6 - idx;
        int copy_count = key_count < remaining_slots ? key_count : remaining_slots;
        for (int i = 0; i < copy_count; i++) {
            out[7 + idx++] = keys[i];
        }

        /* Zero-fill remaining slots */
        for (; idx < 6; idx++) {
            out[7 + idx] = 0x00;
        }
    } else {
        /* Standard mode: original behavior */
        out[5] = modifiers;
        out[6] = 0x00;
        write_key_slots(out, keys, num_keys);
    }

    finalize_packet(out, OP_INPUT_PKT_KEYBOARD_SIZE);
    return OP_INPUT_PKT_KEYBOARD_SIZE;
}

int op_input_build_mouse_rel(uint8_t out[OP_INPUT_PKT_MOUSE_REL_SIZE],
                              uint8_t buttons,
                              int8_t dx,
                              int8_t dy,
                              int8_t wheel) {
    copy_packet_header(out, MS_HDR);
    out[5] = OP_INPUT_MS_MODE_REL;
    out[6] = buttons;
    out[7] = (uint8_t)dx;
    out[8] = (uint8_t)dy;
    out[9] = (uint8_t)wheel;
    finalize_packet(out, OP_INPUT_PKT_MOUSE_REL_SIZE);
    return OP_INPUT_PKT_MOUSE_REL_SIZE;
}

int op_input_build_mouse_abs(uint8_t out[OP_INPUT_PKT_MOUSE_ABS_SIZE],
                              uint8_t buttons,
                              uint16_t x,
                              uint16_t y,
                              int8_t wheel) {
    copy_packet_header(out, MS_ABS_HDR);
    out[5] = OP_INPUT_MS_MODE_ABS;
    out[6] = buttons;
    out[7] = x & 0xFF;
    out[8] = (x >> 8) & 0xFF;
    out[9] = y & 0xFF;
    out[10] = (y >> 8) & 0xFF;
    out[11] = (uint8_t)wheel;
    finalize_packet(out, OP_INPUT_PKT_MOUSE_ABS_SIZE);
    return OP_INPUT_PKT_MOUSE_ABS_SIZE;
}

int op_input_build_press_release(uint8_t out[2 * OP_INPUT_PKT_KEYBOARD_SIZE],
                                  uint8_t modifiers,
                                  uint8_t hid_code,
                                  uint8_t flags) {
    uint8_t keys[6] = { hid_code, 0, 0, 0, 0, 0 };
    uint8_t zeros[6] = { 0 };

    op_input_build_keyboard(out, modifiers, keys, 1, flags);
    op_input_build_keyboard(out + OP_INPUT_PKT_KEYBOARD_SIZE, 0x00, zeros, 0, flags);
    return 2 * OP_INPUT_PKT_KEYBOARD_SIZE;
}
