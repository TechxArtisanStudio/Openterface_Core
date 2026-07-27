#ifndef OPENTERFACE_INPUT_H
#define OPENTERFACE_INPUT_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Modifier bitmask ───────────────────────────────────────────────────── */

#define OP_INPUT_MOD_NONE   0x00
#define OP_INPUT_MOD_LCTRL  0x01
#define OP_INPUT_MOD_LSHIFT 0x02
#define OP_INPUT_MOD_LALT   0x04
#define OP_INPUT_MOD_LGUI   0x08
#define OP_INPUT_MOD_RCTRL  0x10
#define OP_INPUT_MOD_RSHIFT 0x20
#define OP_INPUT_MOD_RALT   0x40
#define OP_INPUT_MOD_RGUI   0x80
#define OP_INPUT_MOD_CTRL   OP_INPUT_MOD_LCTRL
#define OP_INPUT_MOD_SHIFT  OP_INPUT_MOD_LSHIFT
#define OP_INPUT_MOD_ALT    OP_INPUT_MOD_LALT
#define OP_INPUT_MOD_GUI    OP_INPUT_MOD_LGUI

/* ── CH9329 command codes ───────────────────────────────────────────────── */

#define OP_INPUT_CMD_KB       0x02
#define OP_INPUT_CMD_MS_REL   0x05
#define OP_INPUT_CMD_MS_ABS   0x04

/* ── Packet size constants ──────────────────────────────────────────────── */

#define OP_INPUT_PKT_KEYBOARD_SIZE  14  /* 5 header + 8 data + 1 checksum */
#define OP_INPUT_PKT_MOUSE_REL_SIZE 11  /* 5 header + 5 data + 1 checksum */
#define OP_INPUT_PKT_MOUSE_ABS_SIZE 13  /* 5 header + 7 data + 1 checksum */

/* ── Mouse button mask ──────────────────────────────────────────────────── */

#define OP_INPUT_MS_BTN_NONE   0x00
#define OP_INPUT_MS_BTN_LEFT   0x01
#define OP_INPUT_MS_BTN_RIGHT  0x02
#define OP_INPUT_MS_BTN_MIDDLE 0x04

/* ── Parsed types ───────────────────────────────────────────────────────── */

/** Parsed result from op_input_parse_token() and op_input_macro_parse(). */
typedef struct {
    int hid_code;    /* HID usage code, -1 if invalid */
    int modifiers;   /* Modifier bitmask at time of key recognition */
} op_input_parsed_token_t;

/** UTF-8 byte span describing one raw script token inside the original input. */
typedef struct {
    int start_utf8;
    int length_utf8;
} op_input_script_token_span_t;

/* ── HID lookup API ─────────────────────────────────────────────────────── */

/** Map a DOM event.code string to a HID usage code. */
int op_input_hid_code_from_dom_code(const char *dom_code);

/** Look up a HID usage code by key name (e.g. "Enter", "A", "F1"). */
int op_input_hid_code_from_name(const char *key_name);

/** Map a printable ASCII character to its HID usage code. */
int op_input_hid_code_from_char(char c, int *out_needs_shift);

/** Convert a HID usage code to a human-readable label. */
const char *op_input_hid_code_label(int hid_code);

/* ── Token / macro parsing API ──────────────────────────────────────────── */

/** Parse a single token string into a HID code + modifier bitmask. */
op_input_parsed_token_t op_input_parse_token(const char *token);

/** Parse a full macro string with embedded tokens into an array of parsed tokens. */
int op_input_macro_parse(const char *input, op_input_parsed_token_t out[], int max);

/** Tokenize a macro script into raw tag or character spans. */
int op_input_script_tokenize(const char *input, op_input_script_token_span_t out[], int max);

/* ── Packet building API ────────────────────────────────────────────────── */

/** Build a CH9329 keyboard packet. */
int op_input_build_keyboard(uint8_t out[OP_INPUT_PKT_KEYBOARD_SIZE],
                             uint8_t modifiers,
                             const uint8_t keys[],
                             int num_keys);

/** Build a CH9329 relative-mouse packet. */
int op_input_build_mouse_rel(uint8_t out[OP_INPUT_PKT_MOUSE_REL_SIZE],
                              uint8_t buttons,
                              int8_t dx,
                              int8_t dy,
                              int8_t wheel);

/** Build a CH9329 absolute-mouse packet. */
int op_input_build_mouse_abs(uint8_t out[OP_INPUT_PKT_MOUSE_ABS_SIZE],
                              uint8_t buttons,
                              uint16_t x,
                              uint16_t y,
                              int8_t wheel);

/** Build a press+release keyboard sequence (two packets consecutively). */
int op_input_build_press_release(uint8_t out[2 * OP_INPUT_PKT_KEYBOARD_SIZE],
                                  uint8_t modifiers,
                                  uint8_t hid_code);

/* ── Utility API ────────────────────────────────────────────────────────── */

/** Compute the CH9329 checksum for a packet. */
uint8_t op_input_checksum(const uint8_t data[], int len);

/** Format a packet as an uppercase hex string (for debugging). */
void op_input_hex_dump(const uint8_t data[], int len, char out[]);

/* ── Backward compatibility macros (keymod → op_input) ──────────────────── */

#define KM_MOD_NONE   OP_INPUT_MOD_NONE
#define KM_MOD_CTRL   OP_INPUT_MOD_CTRL
#define KM_MOD_SHIFT  OP_INPUT_MOD_SHIFT
#define KM_MOD_ALT    OP_INPUT_MOD_ALT
#define KM_MOD_GUI    OP_INPUT_MOD_GUI
#define KM_CMD_KB       OP_INPUT_CMD_KB
#define KM_CMD_MS_REL   OP_INPUT_CMD_MS_REL
#define KM_CMD_MS_ABS   OP_INPUT_CMD_MS_ABS
#define KM_PKT_KEYBOARD_SIZE  OP_INPUT_PKT_KEYBOARD_SIZE
#define KM_PKT_MOUSE_REL_SIZE OP_INPUT_PKT_MOUSE_REL_SIZE
#define KM_PKT_MOUSE_ABS_SIZE OP_INPUT_PKT_MOUSE_ABS_SIZE
#define KM_MS_BTN_NONE   OP_INPUT_MS_BTN_NONE
#define KM_MS_BTN_LEFT   OP_INPUT_MS_BTN_LEFT
#define KM_MS_BTN_RIGHT  OP_INPUT_MS_BTN_RIGHT
#define KM_MS_BTN_MIDDLE OP_INPUT_MS_BTN_MIDDLE

typedef op_input_parsed_token_t   km_parsed_token_t;
typedef op_input_script_token_span_t km_script_token_span_t;

#define km_hid_code             op_input_hid_code_from_name
#define km_hid_code_for_char    op_input_hid_code_from_char
#define km_hid_code_label       op_input_hid_code_label
#define km_build_keyboard       op_input_build_keyboard
#define km_build_mouse_rel      op_input_build_mouse_rel
#define km_build_mouse_abs      op_input_build_mouse_abs
#define km_build_press_release  op_input_build_press_release
#define km_parse_token          op_input_parse_token
#define km_parse_macro          op_input_macro_parse
#define km_tokenize_script      op_input_script_tokenize
#define km_checksum             op_input_checksum
#define km_hex_dump             op_input_hex_dump

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_INPUT_H */
