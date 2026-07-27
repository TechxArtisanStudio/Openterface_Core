#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "openterface/input.h"

static int failures = 0;
static int total = 0;

#define RUN_TEST(fn) do { \
    int before = failures; \
    fn(); \
    if (failures == before) printf("  PASS  " #fn "\n"); \
    else printf("  FAIL  " #fn "\n"); \
    total++; \
} while(0)

#define ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        fprintf(stderr, "    FAIL: %s: expected %d (0x%02X), got %d (0x%02X)\n", \
                msg, (int)(expected), (int)(expected), (int)(actual), (int)(actual)); \
        failures++; \
    } \
} while(0)

#define ASSERT_NEQ(bad, actual, msg) do { \
    if ((bad) == (actual)) { \
        fprintf(stderr, "    FAIL: %s: expected != %d (0x%02X)\n", \
                msg, (int)(bad), (int)(bad)); \
        failures++; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, "    FAIL: %s: expected '%s', got '%s'\n", msg, expected, actual); \
        failures++; \
    } \
} while(0)

/* ============================================================================
 * 1. Checksum
 * ========================================================================= */

static void test_checksum_zero_data(void) {
    uint8_t data[14] = {0};
    ASSERT_EQ(0x00, op_input_checksum(data, 14), "all zero checksum");
}

static void test_checksum_single_byte(void) {
    uint8_t data[1] = {0x42};
    ASSERT_EQ(0x00, op_input_checksum(data, 1), "single byte (len-1=0, sum=0)");
}

static void test_checksum_overflow_truncation(void) {
    uint8_t data[3] = {0xFF, 0xFF, 0x00};
    ASSERT_EQ(0xFE, op_input_checksum(data, 3), "overflow truncation");
}

static void test_checksum_known_kb_packet(void) {
    uint8_t pkt[14] = {0};
    pkt[0] = 0x57; pkt[1] = 0xAB; pkt[2] = 0x00; pkt[3] = 0x02; pkt[4] = 0x08;
    pkt[7] = 0x04;
    ASSERT_EQ(0x10, op_input_checksum(pkt, 14), "known keyboard checksum");
}

/* ============================================================================
 * 2. Keyboard Packet Building
 * ========================================================================= */

static void test_kb_packet_header(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 0);
    ASSERT_EQ(0x57, pkt[0], "header byte 0");
    ASSERT_EQ(0xAB, pkt[1], "header byte 1");
    ASSERT_EQ(0x00, pkt[2], "header byte 2");
    ASSERT_EQ(OP_INPUT_CMD_KB, pkt[3], "command byte");
    ASSERT_EQ(0x08, pkt[4], "data length");
}

static void test_kb_packet_single_key(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0x04, 0, 0, 0, 0, 0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 1);
    ASSERT_EQ(0x00, pkt[5], "no modifier");
    ASSERT_EQ(0x04, pkt[7], "key A in slot 1");
    ASSERT_EQ(0x00, pkt[8], "slot 2 empty");
    ASSERT_EQ(0x00, pkt[12], "slot 6 empty");
}

static void test_kb_packet_combined_modifiers(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0x04, 0x05, 0, 0, 0, 0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL | OP_INPUT_MOD_SHIFT, keys, 2);
    ASSERT_EQ(0x03, pkt[5], "CTRL|SHIFT = 0x03");
    ASSERT_EQ(0x04, pkt[7], "key A");
    ASSERT_EQ(0x05, pkt[8], "key B");
    ASSERT_EQ(0x00, pkt[9], "slot 3 empty");
}

static void test_kb_packet_all_modifiers(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0x04, 0, 0, 0, 0, 0};
    uint8_t all = OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_LSHIFT | OP_INPUT_MOD_LALT | OP_INPUT_MOD_LGUI
                | OP_INPUT_MOD_RCTRL | OP_INPUT_MOD_RSHIFT | OP_INPUT_MOD_RALT | OP_INPUT_MOD_RGUI;
    op_input_build_keyboard(pkt, all, keys, 1);
    ASSERT_EQ(0xFF, pkt[5], "all modifiers");
}

static void test_kb_packet_six_keys(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 6);
    ASSERT_EQ(0x04, pkt[7],  "slot 1");
    ASSERT_EQ(0x05, pkt[8],  "slot 2");
    ASSERT_EQ(0x06, pkt[9],  "slot 3");
    ASSERT_EQ(0x07, pkt[10], "slot 4");
    ASSERT_EQ(0x08, pkt[11], "slot 5");
    ASSERT_EQ(0x09, pkt[12], "slot 6");
}

static void test_kb_packet_overflow_clamped(void) {
    uint8_t pkt[14];
    uint8_t seven[7] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, seven, 7);
    ASSERT_EQ(0x09, pkt[12], "slot 6 unchanged (7th key dropped)");
}

static void test_kb_packet_zero_keys(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0xFF, 0xFF, 0, 0, 0, 0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 2);
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 0);
    ASSERT_EQ(0x00, pkt[7],  "slot 1 zeroed");
    ASSERT_EQ(0x00, pkt[12], "slot 6 zeroed");
}

static void test_kb_packet_negative_keys(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, -1);
    ASSERT_EQ(0x00, pkt[7], "negative keys treated as 0");
}

static void test_kb_packet_checksum_filled(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0x04, 0, 0, 0, 0, 0};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 1);
    ASSERT_EQ(op_input_checksum(pkt, 14), pkt[13], "checksum auto-filled");
}

static void test_kb_packet_return_length(void) {
    uint8_t pkt[14];
    uint8_t keys[6] = {0};
    int len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 0);
    ASSERT_EQ(OP_INPUT_PKT_KEYBOARD_SIZE, len, "returns 14");
}

/* ============================================================================
 * 3. Press + Release
 * ========================================================================= */

static void test_press_release_basic(void) {
    uint8_t out[28];
    int len = op_input_build_press_release(out, OP_INPUT_MOD_NONE, 0x28);
    ASSERT_EQ(28, len, "press+release length");
    ASSERT_EQ(0x28, out[7], "press key = Enter");
    ASSERT_EQ(0x00, out[5], "press modifier = NONE");
    ASSERT_EQ(0x00, out[14 + 5], "release modifier = 0");
    ASSERT_EQ(0x00, out[14 + 7], "release key = 0");
    ASSERT_EQ(0x00, out[14 + 8], "release slot 2 = 0");
}

static void test_press_release_with_modifier(void) {
    uint8_t out[28];
    op_input_build_press_release(out, OP_INPUT_MOD_SHIFT, 0x04);
    ASSERT_EQ(OP_INPUT_MOD_SHIFT, out[5], "press has SHIFT");
    ASSERT_EQ(0x00, out[14 + 5], "release no modifier");
}

static void test_press_release_both_checksums(void) {
    uint8_t out[28];
    op_input_build_press_release(out, OP_INPUT_MOD_NONE, 0x04);
    ASSERT_EQ(op_input_checksum(out, 14), out[13], "press checksum valid");
    ASSERT_EQ(op_input_checksum(out + 14, 14), out[27], "release checksum valid");
}

/* ============================================================================
 * 4. ASCII Character -> HID Mapping
 * ========================================================================= */

static void test_char_lowercase(void) {
    int shift;
    for (char c = 'a'; c <= 'z'; c++) {
        int hid = op_input_hid_code_from_char(c, &shift);
        ASSERT_NEQ(-1, hid, "lowercase has HID");
        ASSERT_EQ(0, shift, "lowercase no shift");
    }
    ASSERT_EQ(0x04, op_input_hid_code_from_char('a', &shift), "'a' = 0x04");
    ASSERT_EQ(0x1D, op_input_hid_code_from_char('z', &shift), "'z' = 0x1D");
}

static void test_char_uppercase(void) {
    int shift;
    for (char c = 'A'; c <= 'Z'; c++) {
        int hid = op_input_hid_code_from_char(c, &shift);
        ASSERT_NEQ(-1, hid, "uppercase has HID");
        ASSERT_EQ(1, shift, "uppercase needs shift");
    }
}

static void test_char_digits(void) {
    int shift;
    for (char c = '0'; c <= '9'; c++) {
        int hid = op_input_hid_code_from_char(c, &shift);
        ASSERT_NEQ(-1, hid, "digit has HID");
        ASSERT_EQ(0, shift, "digit no shift");
    }
    ASSERT_EQ(0x27, op_input_hid_code_from_char('0', &shift), "'0' = 0x27");
    ASSERT_EQ(0x1E, op_input_hid_code_from_char('1', &shift), "'1' = 0x1E");
    ASSERT_EQ(0x26, op_input_hid_code_from_char('9', &shift), "'9' = 0x26");
}

static void test_char_shifted_symbols(void) {
    int shift;
    ASSERT_EQ(0x1E, op_input_hid_code_from_char('!', &shift), "'!' HID");
    ASSERT_EQ(1, shift, "'!' needs shift");
    ASSERT_EQ(0x1F, op_input_hid_code_from_char('@', &shift), "'@' HID");
    ASSERT_EQ(1, shift, "'@' needs shift");
    ASSERT_EQ(0x20, op_input_hid_code_from_char('#', &shift), "'#' HID");
    ASSERT_EQ(1, shift, "'#' needs shift");
    ASSERT_EQ(0x25, op_input_hid_code_from_char('*', &shift), "'*' HID");
    ASSERT_EQ(1, shift, "'*' needs shift");
}

static void test_char_unshifted_punctuation(void) {
    int shift;
    ASSERT_EQ(0x36, op_input_hid_code_from_char(',', &shift), "',' HID");
    ASSERT_EQ(0, shift, "',' no shift");
    ASSERT_EQ(0x2D, op_input_hid_code_from_char('-', &shift), "'-' HID");
    ASSERT_EQ(0, shift, "'-' no shift");
    ASSERT_EQ(0x37, op_input_hid_code_from_char('.', &shift), "'.' HID");
    ASSERT_EQ(0, shift, "'.' no shift");
    ASSERT_EQ(0x38, op_input_hid_code_from_char('/', &shift), "'/' HID");
    ASSERT_EQ(0, shift, "'/' no shift");
    ASSERT_EQ(0x33, op_input_hid_code_from_char(';', &shift), "';' HID");
    ASSERT_EQ(0, shift, "';' no shift");
    ASSERT_EQ(0x2E, op_input_hid_code_from_char('=', &shift), "'=' HID");
    ASSERT_EQ(0, shift, "'=' no shift");
}

static void test_char_shifted_punctuation(void) {
    int shift;
    ASSERT_EQ(0x33, op_input_hid_code_from_char(':', &shift), "':' HID");
    ASSERT_EQ(1, shift, "':' needs shift");
    ASSERT_EQ(0x34, op_input_hid_code_from_char('"', &shift), "'\"' HID");
    ASSERT_EQ(1, shift, "'\"' needs shift");
    ASSERT_EQ(0x36, op_input_hid_code_from_char('<', &shift), "'<' HID");
    ASSERT_EQ(1, shift, "'<' needs shift");
    ASSERT_EQ(0x37, op_input_hid_code_from_char('>', &shift), "'>' HID");
    ASSERT_EQ(1, shift, "'>' needs shift");
    ASSERT_EQ(0x38, op_input_hid_code_from_char('?', &shift), "'?' HID");
    ASSERT_EQ(1, shift, "'?' needs shift");
}

static void test_char_brackets(void) {
    int shift;
    ASSERT_EQ(0x2F, op_input_hid_code_from_char('[', &shift), "'[' HID");
    ASSERT_EQ(0, shift, "'[' no shift");
    ASSERT_EQ(0x30, op_input_hid_code_from_char(']', &shift), "']' HID");
    ASSERT_EQ(0, shift, "']' no shift");
    ASSERT_EQ(0x2F, op_input_hid_code_from_char('{', &shift), "'{' HID");
    ASSERT_EQ(1, shift, "'{' needs shift");
    ASSERT_EQ(0x30, op_input_hid_code_from_char('}', &shift), "'}' HID");
    ASSERT_EQ(1, shift, "'}' needs shift");
}

static void test_char_special_keys(void) {
    int shift;
    ASSERT_EQ(0x2C, op_input_hid_code_from_char(' ', &shift), "' ' (space) HID");
    ASSERT_EQ(0, shift, "space no shift");
    ASSERT_EQ(0x35, op_input_hid_code_from_char('`', &shift), "'`' HID");
    ASSERT_EQ(0, shift, "'`' no shift");
    ASSERT_EQ(0x34, op_input_hid_code_from_char('\'', &shift), "'\\'' HID");
    ASSERT_EQ(0, shift, "'\\'' no shift");
    ASSERT_EQ(0x64, op_input_hid_code_from_char('\\', &shift), "'\\\\' HID");
    ASSERT_EQ(0, shift, "'\\\\' no shift");
}

static void test_char_boundary(void) {
    int shift;
    ASSERT_NEQ(-1, op_input_hid_code_from_char(' ', &shift), "0x20 valid");
    ASSERT_NEQ(-1, op_input_hid_code_from_char('~', &shift), "0x7E valid");
    ASSERT_EQ(0x35, op_input_hid_code_from_char('~', &shift), "'~' HID");
    ASSERT_EQ(1, shift, "'~' needs shift");
}

static void test_char_out_of_range(void) {
    int shift;
    ASSERT_EQ(-1, op_input_hid_code_from_char('\n', &shift), "newline invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\t', &shift), "tab invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\r', &shift), "CR invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\x00', &shift), "NULL char invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\x1F', &shift), "0x1F invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\x7F', &shift), "DEL invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\x80', &shift), "0x80 invalid");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\xFF', &shift), "0xFF invalid");
}

static void test_char_null_shift_ptr(void) {
    int hid = op_input_hid_code_from_char('A', NULL);
    ASSERT_EQ(0x04, hid, "'A' HID with NULL shift ptr");
}

/* ============================================================================
 * 5. Named Key -> HID Mapping
 * ========================================================================= */

static void test_name_all_letters(void) {
    ASSERT_EQ(0x04, op_input_hid_code_from_name("A"), "A");
    ASSERT_EQ(0x05, op_input_hid_code_from_name("B"), "B");
    ASSERT_EQ(0x06, op_input_hid_code_from_name("C"), "C");
    ASSERT_EQ(0x0D, op_input_hid_code_from_name("J"), "J");
    ASSERT_EQ(0x1D, op_input_hid_code_from_name("Z"), "Z");
}

static void test_name_all_digits(void) {
    ASSERT_EQ(0x27, op_input_hid_code_from_name("0"), "0");
    ASSERT_EQ(0x1E, op_input_hid_code_from_name("1"), "1");
    ASSERT_EQ(0x1F, op_input_hid_code_from_name("2"), "2");
    ASSERT_EQ(0x26, op_input_hid_code_from_name("9"), "9");
}

static void test_name_action_keys(void) {
    ASSERT_EQ(0x28, op_input_hid_code_from_name("Enter"), "Enter");
    ASSERT_EQ(0x29, op_input_hid_code_from_name("Escape"), "Escape");
    ASSERT_EQ(0x2A, op_input_hid_code_from_name("Backspace"), "Backspace");
    ASSERT_EQ(0x2B, op_input_hid_code_from_name("Tab"), "Tab");
    ASSERT_EQ(0x2C, op_input_hid_code_from_name("Space"), "Space");
    ASSERT_EQ(0x39, op_input_hid_code_from_name("Caps"), "Caps");
}

static void test_name_function_keys(void) {
    ASSERT_EQ(0x3A, op_input_hid_code_from_name("F1"), "F1");
    ASSERT_EQ(0x3B, op_input_hid_code_from_name("F2"), "F2");
    ASSERT_EQ(0x3F, op_input_hid_code_from_name("F6"), "F6");
    ASSERT_EQ(0x44, op_input_hid_code_from_name("F11"), "F11");
    ASSERT_EQ(0x45, op_input_hid_code_from_name("F12"), "F12");
}

static void test_name_arrow_keys(void) {
    ASSERT_EQ(0x4F, op_input_hid_code_from_name("Right"), "Right");
    ASSERT_EQ(0x50, op_input_hid_code_from_name("Left"), "Left");
    ASSERT_EQ(0x51, op_input_hid_code_from_name("Down"), "Down");
    ASSERT_EQ(0x52, op_input_hid_code_from_name("Up"), "Up");
}

static void test_name_navigation_keys(void) {
    ASSERT_EQ(0x49, op_input_hid_code_from_name("Insert"), "Insert");
    ASSERT_EQ(0x4A, op_input_hid_code_from_name("Home"), "Home");
    ASSERT_EQ(0x4B, op_input_hid_code_from_name("PageUp"), "PageUp");
    ASSERT_EQ(0x4C, op_input_hid_code_from_name("Delete"), "Delete");
    ASSERT_EQ(0x4D, op_input_hid_code_from_name("End"), "End");
    ASSERT_EQ(0x4E, op_input_hid_code_from_name("PageDown"), "PageDown");
}

static void test_name_navigation_aliases(void) {
    ASSERT_EQ(0x4B, op_input_hid_code_from_name("PgUp"), "PgUp alias");
    ASSERT_EQ(0x4E, op_input_hid_code_from_name("PgDn"), "PgDn alias");
}

static void test_name_modifier_keys(void) {
    ASSERT_EQ(0xE0, op_input_hid_code_from_name("Ctrl"), "Ctrl");
    ASSERT_EQ(0xE1, op_input_hid_code_from_name("Shift"), "Shift");
    ASSERT_EQ(0xE2, op_input_hid_code_from_name("Alt"), "Alt");
    ASSERT_EQ(0xE3, op_input_hid_code_from_name("Cmd"), "Cmd");
    ASSERT_EQ(0xE3, op_input_hid_code_from_name("Win"), "Win");
}

static void test_name_numpad(void) {
    ASSERT_EQ(0x62, op_input_hid_code_from_name("Numpad0"), "Numpad0");
    ASSERT_EQ(0x59, op_input_hid_code_from_name("Numpad1"), "Numpad1");
    ASSERT_EQ(0x61, op_input_hid_code_from_name("Numpad9"), "Numpad9");
    ASSERT_EQ(0x63, op_input_hid_code_from_name("NumpadDot"), "NumpadDot");
    ASSERT_EQ(0x54, op_input_hid_code_from_name("NumpadSlash"), "NumpadSlash");
    ASSERT_EQ(0x55, op_input_hid_code_from_name("NumpadAsterisk"), "NumpadAsterisk");
    ASSERT_EQ(0x56, op_input_hid_code_from_name("NumpadMinus"), "NumpadMinus");
    ASSERT_EQ(0x57, op_input_hid_code_from_name("NumpadPlus"), "NumpadPlus");
    ASSERT_EQ(0x58, op_input_hid_code_from_name("NumpadEnter"), "NumpadEnter");
    ASSERT_EQ(0x67, op_input_hid_code_from_name("NumpadEquals"), "NumpadEquals");
    ASSERT_EQ(0x53, op_input_hid_code_from_name("NumLock"), "NumLock");
}

static void test_name_system_keys(void) {
    ASSERT_EQ(0x65, op_input_hid_code_from_name("App"), "App");
    ASSERT_EQ(0x47, op_input_hid_code_from_name("ScrollLock"), "ScrollLock");
    ASSERT_EQ(0x46, op_input_hid_code_from_name("PrtSc"), "PrtSc");
    ASSERT_EQ(0x48, op_input_hid_code_from_name("Pause"), "Pause");
}

static void test_name_invalid(void) {
    ASSERT_EQ(-1, op_input_hid_code_from_name("NoSuchKey"), "unknown key");
    ASSERT_EQ(-1, op_input_hid_code_from_name(""), "empty string");
    ASSERT_EQ(-1, op_input_hid_code_from_name(NULL), "NULL");
    ASSERT_EQ(-1, op_input_hid_code_from_name("f1"), "lowercase f1");
    ASSERT_EQ(-1, op_input_hid_code_from_name("enter"), "lowercase enter");
}

/* ============================================================================
 * 6. HID Code -> Label
 * ========================================================================= */

static void test_label_known_keys(void) {
    ASSERT_STR_EQ("Enter", op_input_hid_code_label(0x28), "label Enter");
    ASSERT_STR_EQ("A", op_input_hid_code_label(0x04), "label A");
    ASSERT_STR_EQ("F1", op_input_hid_code_label(0x3A), "label F1");
    ASSERT_STR_EQ("Ctrl", op_input_hid_code_label(0xE0), "label Ctrl");
}

static void test_label_unknown(void) {
    ASSERT_STR_EQ("Unknown", op_input_hid_code_label(0xFF), "label 0xFF");
    ASSERT_STR_EQ("Unknown", op_input_hid_code_label(0x00), "label 0x00");
}

/* ============================================================================
 * 7. DOM event.code -> HID Mapping
 * ========================================================================= */

static void test_dom_letters(void) {
    ASSERT_EQ(0x04, op_input_hid_code_from_dom_code("KeyA"), "KeyA");
    ASSERT_EQ(0x05, op_input_hid_code_from_dom_code("KeyB"), "KeyB");
    ASSERT_EQ(0x1D, op_input_hid_code_from_dom_code("KeyZ"), "KeyZ");
}

static void test_dom_digits(void) {
    ASSERT_EQ(0x27, op_input_hid_code_from_dom_code("Digit0"), "Digit0");
    ASSERT_EQ(0x1E, op_input_hid_code_from_dom_code("Digit1"), "Digit1");
    ASSERT_EQ(0x26, op_input_hid_code_from_dom_code("Digit9"), "Digit9");
}

static void test_dom_punctuation(void) {
    ASSERT_EQ(0x2D, op_input_hid_code_from_dom_code("Minus"), "Minus");
    ASSERT_EQ(0x2E, op_input_hid_code_from_dom_code("Equal"), "Equal");
    ASSERT_EQ(0x2F, op_input_hid_code_from_dom_code("BracketLeft"), "BracketLeft");
    ASSERT_EQ(0x30, op_input_hid_code_from_dom_code("BracketRight"), "BracketRight");
    ASSERT_EQ(0x31, op_input_hid_code_from_dom_code("Backslash"), "Backslash");
    ASSERT_EQ(0x64, op_input_hid_code_from_dom_code("IntlBackslash"), "IntlBackslash");
    ASSERT_EQ(0x33, op_input_hid_code_from_dom_code("Semicolon"), "Semicolon");
    ASSERT_EQ(0x34, op_input_hid_code_from_dom_code("Quote"), "Quote");
    ASSERT_EQ(0x35, op_input_hid_code_from_dom_code("Backquote"), "Backquote");
    ASSERT_EQ(0x36, op_input_hid_code_from_dom_code("Comma"), "Comma");
    ASSERT_EQ(0x37, op_input_hid_code_from_dom_code("Period"), "Period");
    ASSERT_EQ(0x38, op_input_hid_code_from_dom_code("Slash"), "Slash");
}

static void test_dom_action_keys(void) {
    ASSERT_EQ(0x28, op_input_hid_code_from_dom_code("Enter"), "Enter");
    ASSERT_EQ(0x29, op_input_hid_code_from_dom_code("Escape"), "Escape");
    ASSERT_EQ(0x2A, op_input_hid_code_from_dom_code("Backspace"), "Backspace");
    ASSERT_EQ(0x2B, op_input_hid_code_from_dom_code("Tab"), "Tab");
    ASSERT_EQ(0x2C, op_input_hid_code_from_dom_code("Space"), "Space");
    ASSERT_EQ(0x39, op_input_hid_code_from_dom_code("CapsLock"), "CapsLock");
}

static void test_dom_navigation(void) {
    ASSERT_EQ(0x49, op_input_hid_code_from_dom_code("Insert"), "Insert");
    ASSERT_EQ(0x4A, op_input_hid_code_from_dom_code("Home"), "Home");
    ASSERT_EQ(0x4B, op_input_hid_code_from_dom_code("PageUp"), "PageUp");
    ASSERT_EQ(0x4C, op_input_hid_code_from_dom_code("Delete"), "Delete");
    ASSERT_EQ(0x4D, op_input_hid_code_from_dom_code("End"), "End");
    ASSERT_EQ(0x4E, op_input_hid_code_from_dom_code("PageDown"), "PageDown");
}

static void test_dom_arrows(void) {
    ASSERT_EQ(0x4F, op_input_hid_code_from_dom_code("ArrowRight"), "ArrowRight");
    ASSERT_EQ(0x50, op_input_hid_code_from_dom_code("ArrowLeft"), "ArrowLeft");
    ASSERT_EQ(0x51, op_input_hid_code_from_dom_code("ArrowDown"), "ArrowDown");
    ASSERT_EQ(0x52, op_input_hid_code_from_dom_code("ArrowUp"), "ArrowUp");
}

static void test_dom_function_keys(void) {
    ASSERT_EQ(0x3A, op_input_hid_code_from_dom_code("F1"), "F1");
    ASSERT_EQ(0x45, op_input_hid_code_from_dom_code("F12"), "F12");
    ASSERT_EQ(0x46, op_input_hid_code_from_dom_code("PrintScreen"), "PrintScreen");
    ASSERT_EQ(0x47, op_input_hid_code_from_dom_code("ScrollLock"), "ScrollLock");
    ASSERT_EQ(0x48, op_input_hid_code_from_dom_code("Pause"), "Pause");
}

static void test_dom_numpad(void) {
    ASSERT_EQ(0x53, op_input_hid_code_from_dom_code("NumLock"), "NumLock");
    ASSERT_EQ(0x54, op_input_hid_code_from_dom_code("NumpadDivide"), "NumpadDivide");
    ASSERT_EQ(0x55, op_input_hid_code_from_dom_code("NumpadMultiply"), "NumpadMultiply");
    ASSERT_EQ(0x56, op_input_hid_code_from_dom_code("NumpadSubtract"), "NumpadSubtract");
    ASSERT_EQ(0x57, op_input_hid_code_from_dom_code("NumpadAdd"), "NumpadAdd");
    ASSERT_EQ(0x58, op_input_hid_code_from_dom_code("NumpadEnter"), "NumpadEnter");
    ASSERT_EQ(0x62, op_input_hid_code_from_dom_code("Numpad0"), "Numpad0");
    ASSERT_EQ(0x61, op_input_hid_code_from_dom_code("Numpad9"), "Numpad9");
    ASSERT_EQ(0x63, op_input_hid_code_from_dom_code("NumpadDecimal"), "NumpadDecimal");
    ASSERT_EQ(0x67, op_input_hid_code_from_dom_code("NumpadEqual"), "NumpadEqual");
}

static void test_dom_left_modifiers(void) {
    ASSERT_EQ(0xE0, op_input_hid_code_from_dom_code("ControlLeft"), "ControlLeft");
    ASSERT_EQ(0xE1, op_input_hid_code_from_dom_code("ShiftLeft"), "ShiftLeft");
    ASSERT_EQ(0xE2, op_input_hid_code_from_dom_code("AltLeft"), "AltLeft");
    ASSERT_EQ(0xE3, op_input_hid_code_from_dom_code("MetaLeft"), "MetaLeft");
}

static void test_dom_right_modifiers(void) {
    ASSERT_EQ(0xE4, op_input_hid_code_from_dom_code("ControlRight"), "ControlRight");
    ASSERT_EQ(0xE5, op_input_hid_code_from_dom_code("ShiftRight"), "ShiftRight");
    ASSERT_EQ(0xE6, op_input_hid_code_from_dom_code("AltRight"), "AltRight");
    ASSERT_EQ(0xE7, op_input_hid_code_from_dom_code("MetaRight"), "MetaRight");
}

static void test_dom_left_right_differ(void) {
    int left  = op_input_hid_code_from_dom_code("ControlLeft");
    int right = op_input_hid_code_from_dom_code("ControlRight");
    ASSERT_NEQ(left, right, "left != right modifier HID");
}

static void test_dom_invalid(void) {
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code("BrowserBack"), "unknown DOM code");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code("GamepadButton0"), "unknown gamepad");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code(""), "empty");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code(NULL), "NULL");
}

static void test_dom_vs_name_consistency(void) {
    ASSERT_EQ(op_input_hid_code_from_name("A"),
              op_input_hid_code_from_dom_code("KeyA"), "KeyA consistency");
    ASSERT_EQ(op_input_hid_code_from_name("Enter"),
              op_input_hid_code_from_dom_code("Enter"), "Enter consistency");
    ASSERT_EQ(op_input_hid_code_from_name("F1"),
              op_input_hid_code_from_dom_code("F1"), "F1 consistency");
    ASSERT_EQ(op_input_hid_code_from_name("Up"),
              op_input_hid_code_from_dom_code("ArrowUp"), "ArrowUp/Up consistency");
}

/* ============================================================================
 * 8. Single Token Parser
 * ========================================================================= */

static void test_token_single_char(void) {
    op_input_parsed_token_t t = op_input_parse_token("A");
    ASSERT_EQ(0x04, t.hid_code, "token 'A'");
    ASSERT_EQ(0, t.modifiers, "token 'A' no modifier");
}

static void test_token_special_tags(void) {
    ASSERT_EQ(0x28, op_input_parse_token("<ENTER>").hid_code, "<ENTER>");
    ASSERT_EQ(0x29, op_input_parse_token("<ESC>").hid_code, "<ESC>");
    ASSERT_EQ(0x2A, op_input_parse_token("<BACK>").hid_code, "<BACK>");
    ASSERT_EQ(0x2B, op_input_parse_token("<TAB>").hid_code, "<TAB>");
    ASSERT_EQ(0x2C, op_input_parse_token("<SPACE>").hid_code, "<SPACE>");
}

static void test_token_arrow_tags(void) {
    ASSERT_EQ(0x50, op_input_parse_token("<LEFT>").hid_code, "<LEFT>");
    ASSERT_EQ(0x4F, op_input_parse_token("<RIGHT>").hid_code, "<RIGHT>");
    ASSERT_EQ(0x52, op_input_parse_token("<UP>").hid_code, "<UP>");
    ASSERT_EQ(0x51, op_input_parse_token("<DOWN>").hid_code, "<DOWN>");
}

static void test_token_edit_tags(void) {
    ASSERT_EQ(0x4C, op_input_parse_token("<DEL>").hid_code, "<DEL>");
    ASSERT_EQ(0x4C, op_input_parse_token("<DELETE>").hid_code, "<DELETE>");
    ASSERT_EQ(0x49, op_input_parse_token("<INS>").hid_code, "<INS>");
    ASSERT_EQ(0x49, op_input_parse_token("<INSERT>").hid_code, "<INSERT>");
}

static void test_token_nav_tags(void) {
    ASSERT_EQ(0x4A, op_input_parse_token("<HOME>").hid_code, "<HOME>");
    ASSERT_EQ(0x4D, op_input_parse_token("<END>").hid_code, "<END>");
    ASSERT_EQ(0x4B, op_input_parse_token("<PGUP>").hid_code, "<PGUP>");
    ASSERT_EQ(0x4B, op_input_parse_token("<PAGEUP>").hid_code, "<PAGEUP>");
    ASSERT_EQ(0x4E, op_input_parse_token("<PGDN>").hid_code, "<PGDN>");
    ASSERT_EQ(0x4E, op_input_parse_token("<PAGEDOWN>").hid_code, "<PAGEDOWN>");
}

static void test_token_function_keys(void) {
    ASSERT_EQ(0x3A, op_input_parse_token("<F1>").hid_code, "<F1>");
    ASSERT_EQ(0x3B, op_input_parse_token("<F2>").hid_code, "<F2>");
    ASSERT_EQ(0x45, op_input_parse_token("<F12>").hid_code, "<F12>");
}

static void test_token_modifier_tags_ignored(void) {
    ASSERT_EQ(-1, op_input_parse_token("<CTRL>").hid_code, "<CTRL> no key");
    ASSERT_EQ(-1, op_input_parse_token("</CTRL>").hid_code, "</CTRL> no key");
    ASSERT_EQ(-1, op_input_parse_token("<SHIFT>").hid_code, "<SHIFT> no key");
    ASSERT_EQ(-1, op_input_parse_token("<ALT>").hid_code, "<ALT> no key");
    ASSERT_EQ(-1, op_input_parse_token("<CMD>").hid_code, "<CMD> no key");
    ASSERT_EQ(-1, op_input_parse_token("<WIN>").hid_code, "<WIN> no key");
}

static void test_token_delay_tags(void) {
    ASSERT_EQ(-1, op_input_parse_token("<DELAY1S>").hid_code, "<DELAY1S> ignored");
    ASSERT_EQ(-1, op_input_parse_token("<DELAY500MS>").hid_code, "<DELAY500MS> ignored");
    ASSERT_EQ(-1, op_input_parse_token("<DELAY>").hid_code, "<DELAY> ignored");
}

static void test_token_invalid(void) {
    ASSERT_EQ(-1, op_input_parse_token("<UNKNOWN>").hid_code, "unknown tag");
    ASSERT_EQ(-1, op_input_parse_token("???").hid_code, "garbage");
    ASSERT_EQ(-1, op_input_parse_token("").hid_code, "empty");
    ASSERT_EQ(-1, op_input_parse_token(NULL).hid_code, "NULL");
}

/* ============================================================================
 * 9. Macro Script Parser
 * ========================================================================= */

static void test_macro_ctrl_c(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CTRL>C</CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "Ctrl+C count");
    ASSERT_EQ(0x06, tokens[0].hid_code, "C key");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl modifier");
}

static void test_macro_cmd_v(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CMD>V</CMD>", tokens, 32);
    ASSERT_EQ(1, n, "Cmd+V count");
    ASSERT_EQ(0x19, tokens[0].hid_code, "V key");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[0].modifiers, "GUI modifier");
}

static void test_macro_win_r(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<WIN>R</WIN>", tokens, 32);
    ASSERT_EQ(1, n, "Win+R count");
    ASSERT_EQ(0x15, tokens[0].hid_code, "R key");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[0].modifiers, "GUI modifier");
}

static void test_macro_ctrl_alt_del(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CTRL><ALT><DEL></ALT></CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "Ctrl+Alt+Del count");
    ASSERT_EQ(0x4C, tokens[0].hid_code, "Del key");
    ASSERT_EQ(OP_INPUT_MOD_CTRL | OP_INPUT_MOD_ALT, tokens[0].modifiers, "Ctrl|Alt");
}

static void test_macro_cmd_shift_z(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CMD><SHIFT>Z</SHIFT></CMD>", tokens, 32);
    ASSERT_EQ(1, n, "Cmd+Shift+Z count");
    ASSERT_EQ(0x1D, tokens[0].hid_code, "Z key");
    ASSERT_EQ(OP_INPUT_MOD_GUI | OP_INPUT_MOD_SHIFT, tokens[0].modifiers, "GUI|Shift");
}

static void test_macro_plain_text(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("abc", tokens, 32);
    ASSERT_EQ(3, n, "'abc' count");
    ASSERT_EQ(0x04, tokens[0].hid_code, "a");
    ASSERT_EQ(0x05, tokens[1].hid_code, "b");
    ASSERT_EQ(0x06, tokens[2].hid_code, "c");
    ASSERT_EQ(0, tokens[0].modifiers, "a no modifier");
}

static void test_macro_case_insensitive(void) {
    op_input_parsed_token_t tokens_lo[32], tokens_up[32];
    op_input_macro_parse("abc", tokens_lo, 32);
    op_input_macro_parse("ABC", tokens_up, 32);
    ASSERT_EQ(tokens_lo[0].hid_code, tokens_up[0].hid_code, "A vs a same HID");
    ASSERT_EQ(tokens_lo[1].hid_code, tokens_up[1].hid_code, "B vs b same HID");
    ASSERT_EQ(tokens_lo[2].hid_code, tokens_up[2].hid_code, "C vs c same HID");
}

static void test_macro_shifted_symbol(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("Hi!", tokens, 32);
    ASSERT_EQ(3, n, "'Hi!' count");
    ASSERT_EQ(0, tokens[0].modifiers & OP_INPUT_MOD_SHIFT, "'H' no shift");
    ASSERT_NEQ(0, tokens[2].modifiers & OP_INPUT_MOD_SHIFT, "'!' has shift");
}

static void test_macro_special_sequence(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<F1><ENTER>", tokens, 32);
    ASSERT_EQ(2, n, "F1+Enter count");
    ASSERT_EQ(0x3A, tokens[0].hid_code, "F1");
    ASSERT_EQ(0x28, tokens[1].hid_code, "Enter");
}

static void test_macro_delay_skipped(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<DELAY1S><CTRL>A</CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "delay + Ctrl+A count");
    ASSERT_EQ(0x04, tokens[0].hid_code, "A after delay");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl preserved");
}

static void test_macro_modifier_persists(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CMD>AB</CMD>", tokens, 32);
    ASSERT_EQ(2, n, "Cmd+AB count");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[0].modifiers, "A has GUI");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[1].modifiers, "B has GUI");
}

static void test_macro_null_empty(void) {
    op_input_parsed_token_t tokens[32];
    ASSERT_EQ(0, op_input_macro_parse(NULL, tokens, 32), "NULL input");
    ASSERT_EQ(0, op_input_macro_parse("", tokens, 32), "empty input");
    ASSERT_EQ(0, op_input_macro_parse("abc", NULL, 32), "NULL output");
    ASSERT_EQ(0, op_input_macro_parse("abc", tokens, 0), "max=0");
    ASSERT_EQ(0, op_input_macro_parse("abc", tokens, -1), "max<0");
}

static void test_macro_max_truncation(void) {
    op_input_parsed_token_t tokens[3];
    int n = op_input_macro_parse("abcdefghij", tokens, 3);
    ASSERT_EQ(3, n, "max=3 truncates");
    ASSERT_EQ(0x04, tokens[0].hid_code, "a");
    ASSERT_EQ(0x05, tokens[1].hid_code, "b");
    ASSERT_EQ(0x06, tokens[2].hid_code, "c");
}

static void test_macro_unclosed_modifier(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CTRL>AB", tokens, 32);
    ASSERT_EQ(2, n, "unclosed Ctrl count");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "A has Ctrl");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[1].modifiers, "B has Ctrl");
}

static void test_macro_close_without_open(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("A</CTRL>B", tokens, 32);
    ASSERT_EQ(2, n, "close-without-open count");
    ASSERT_EQ(0, tokens[0].modifiers, "A no modifier");
    ASSERT_EQ(0, tokens[1].modifiers, "B no modifier");
}

static void test_macro_double_modifier_idempotent(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CTRL><CTRL>A</CTRL></CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "double Ctrl count");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl still 0x01");
}

static void test_macro_complex_mixed(void) {
    op_input_parsed_token_t tokens[32];
    int n = op_input_macro_parse("<CTRL>cd /tmp<ENTER></CTRL>", tokens, 32);
    ASSERT_EQ(8, n, "complex mixed count");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "c has Ctrl");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[7].modifiers, "Enter has Ctrl");
    ASSERT_EQ(0x28, tokens[7].hid_code, "Enter key");
}

/* ============================================================================
 * 10. Script Tokenizer
 * ========================================================================= */

static void test_tokenize_plain_text(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("abc", spans, 16);
    ASSERT_EQ(3, n, "plain text count");
    ASSERT_EQ(1, spans[0].length_utf8, "a length");
    ASSERT_EQ(1, spans[1].length_utf8, "b length");
    ASSERT_EQ(1, spans[2].length_utf8, "c length");
}

static void test_tokenize_single_tag(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("<CTRL>", spans, 16);
    ASSERT_EQ(1, n, "single tag count");
    ASSERT_EQ(6, spans[0].length_utf8, "<CTRL> length");
}

static void test_tokenize_mixed(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("A<ENTER>B", spans, 16);
    ASSERT_EQ(3, n, "mixed count");
    ASSERT_EQ(1, spans[0].length_utf8, "A");
    ASSERT_EQ(7, spans[1].length_utf8, "<ENTER>");
    ASSERT_EQ(1, spans[2].length_utf8, "B");
}

static void test_tokenize_closing_tag(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("</CTRL>", spans, 16);
    ASSERT_EQ(1, n, "closing tag count");
    ASSERT_EQ(7, spans[0].length_utf8, "</CTRL> length");
}

static void test_tokenize_utf8_multibyte(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("\xe4\xb8\xad", spans, 16);
    ASSERT_EQ(1, n, "UTF-8 char count");
    ASSERT_EQ(3, spans[0].length_utf8, "UTF-8 length");
}

static void test_tokenize_null(void) {
    op_input_script_token_span_t spans[16];
    ASSERT_EQ(0, op_input_script_tokenize(NULL, spans, 16), "NULL input");
    ASSERT_EQ(0, op_input_script_tokenize("abc", NULL, 16), "NULL output");
    ASSERT_EQ(0, op_input_script_tokenize("abc", spans, 0), "max=0");
}

static void test_tokenize_max_truncation(void) {
    op_input_script_token_span_t spans[1];
    int n = op_input_script_tokenize("abc", spans, 1);
    ASSERT_EQ(1, n, "max=1");
}

static void test_tokenize_illegal_tag(void) {
    op_input_script_token_span_t spans[16];
    int n = op_input_script_tokenize("<abc", spans, 16);
    ASSERT_EQ(4, n, "illegal tag as chars");
}

/* ============================================================================
 * 11. Modifier Bitmask
 * ========================================================================= */

static void test_modifier_constants(void) {
    ASSERT_EQ(0x00, OP_INPUT_MOD_NONE,   "NONE = 0x00");
    ASSERT_EQ(0x01, OP_INPUT_MOD_LCTRL,  "LCTRL = 0x01");
    ASSERT_EQ(0x02, OP_INPUT_MOD_LSHIFT, "LSHIFT = 0x02");
    ASSERT_EQ(0x04, OP_INPUT_MOD_LALT,   "LALT = 0x04");
    ASSERT_EQ(0x08, OP_INPUT_MOD_LGUI,   "LGUI = 0x08");
    ASSERT_EQ(0x10, OP_INPUT_MOD_RCTRL,  "RCTRL = 0x10");
    ASSERT_EQ(0x20, OP_INPUT_MOD_RSHIFT, "RSHIFT = 0x20");
    ASSERT_EQ(0x40, OP_INPUT_MOD_RALT,   "RALT = 0x40");
    ASSERT_EQ(0x80, OP_INPUT_MOD_RGUI,   "RGUI = 0x80");
}

static void test_modifier_aliases(void) {
    ASSERT_EQ(OP_INPUT_MOD_LCTRL,  OP_INPUT_MOD_CTRL,  "CTRL alias");
    ASSERT_EQ(OP_INPUT_MOD_LSHIFT, OP_INPUT_MOD_SHIFT, "SHIFT alias");
    ASSERT_EQ(OP_INPUT_MOD_LALT,   OP_INPUT_MOD_ALT,   "ALT alias");
    ASSERT_EQ(OP_INPUT_MOD_LGUI,   OP_INPUT_MOD_GUI,   "GUI alias");
}

static void test_modifier_non_overlapping(void) {
    int all = OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_LSHIFT | OP_INPUT_MOD_LALT | OP_INPUT_MOD_LGUI
            | OP_INPUT_MOD_RCTRL | OP_INPUT_MOD_RSHIFT | OP_INPUT_MOD_RALT | OP_INPUT_MOD_RGUI;
    ASSERT_EQ(0xFF, all, "all mods = 0xFF");
    ASSERT_EQ(0, OP_INPUT_MOD_LCTRL & OP_INPUT_MOD_LSHIFT, "CTRL & SHIFT = 0");
    ASSERT_EQ(0, OP_INPUT_MOD_LALT  & OP_INPUT_MOD_LGUI,   "ALT & GUI = 0");
}

/* ============================================================================
 * 12. Mouse Packets
 * ========================================================================= */

static void test_mouse_rel_left_click(void) {
    uint8_t pkt[11];
    int len = op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT, 0, 0, 0);
    ASSERT_EQ(11, len, "mouse rel length");
    ASSERT_EQ(0x01, pkt[5], "relative mode");
    ASSERT_EQ(OP_INPUT_MS_BTN_LEFT, pkt[6], "left button");
    ASSERT_EQ(0, pkt[7], "dx=0");
    ASSERT_EQ(0, pkt[8], "dy=0");
    ASSERT_EQ(0, pkt[9], "wheel=0");
}

static void test_mouse_rel_movement(void) {
    uint8_t pkt[11];
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 50, -30, 0);
    ASSERT_EQ(50, (int8_t)pkt[7], "dx=50");
    ASSERT_EQ(-30, (int8_t)pkt[8], "dy=-30");
}

static void test_mouse_rel_scroll(void) {
    uint8_t pkt[11];
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 3);
    ASSERT_EQ(3, pkt[9], "wheel=3");
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, -5);
    ASSERT_EQ(-5, (int8_t)pkt[9], "wheel=-5");
}

static void test_mouse_rel_all_buttons(void) {
    uint8_t pkt[11];
    op_input_build_mouse_rel(pkt,
        OP_INPUT_MS_BTN_LEFT | OP_INPUT_MS_BTN_RIGHT | OP_INPUT_MS_BTN_MIDDLE, 0, 0, 0);
    ASSERT_EQ(0x07, pkt[6], "all buttons");
}

static void test_mouse_abs_coordinates(void) {
    uint8_t pkt[13];
    int len = op_input_build_mouse_abs(pkt, OP_INPUT_MS_BTN_NONE, 0x0123, 0x0456, -1);
    ASSERT_EQ(13, len, "mouse abs length");
    ASSERT_EQ(0x02, pkt[5], "absolute mode");
    ASSERT_EQ(0x23, pkt[7], "x low");
    ASSERT_EQ(0x01, pkt[8], "x high");
    ASSERT_EQ(0x56, pkt[9], "y low");
    ASSERT_EQ(0x04, pkt[10], "y high");
    ASSERT_EQ(-1, (int8_t)pkt[11], "wheel=-1");
}

static void test_mouse_abs_header(void) {
    uint8_t pkt[13];
    op_input_build_mouse_abs(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 0);
    ASSERT_EQ(0x57, pkt[0], "header 0");
    ASSERT_EQ(0xAB, pkt[1], "header 1");
    ASSERT_EQ(OP_INPUT_CMD_MS_ABS, pkt[3], "abs cmd");
    ASSERT_EQ(0x07, pkt[4], "data len");
}

static void test_mouse_checksum_valid(void) {
    uint8_t rpkt[11], apkt[13];
    op_input_build_mouse_rel(rpkt, OP_INPUT_MS_BTN_LEFT, 10, -20, 3);
    op_input_build_mouse_abs(apkt, OP_INPUT_MS_BTN_RIGHT, 500, 400, 0);
    ASSERT_EQ(op_input_checksum(rpkt, 11), rpkt[10], "rel checksum");
    ASSERT_EQ(op_input_checksum(apkt, 13), apkt[12], "abs checksum");
}

/* ============================================================================
 * 13. Hex Dump
 * ========================================================================= */

static void test_hex_dump_standard(void) {
    uint8_t data[] = {0x57, 0xAB, 0x00, 0x02};
    char buf[32];
    op_input_hex_dump(data, 4, buf);
    ASSERT_STR_EQ("57AB0002", buf, "hex dump");
}

static void test_hex_dump_empty(void) {
    uint8_t data[] = {0};
    char buf[32];
    op_input_hex_dump(data, 0, buf);
    ASSERT_STR_EQ("", buf, "empty hex dump");
}

static void test_hex_dump_single_byte(void) {
    uint8_t data[] = {0xFF};
    char buf[32];
    op_input_hex_dump(data, 1, buf);
    ASSERT_STR_EQ("FF", buf, "single byte hex");
}

static void test_hex_dump_all_zeros(void) {
    uint8_t data[] = {0x00, 0x00, 0x00};
    char buf[32];
    op_input_hex_dump(data, 3, buf);
    ASSERT_STR_EQ("000000", buf, "all zeros hex");
}

/* ============================================================================
 * 14. Backward Compat Macros (keymod -> op_input)
 * ========================================================================= */

static void test_backward_compat_constants(void) {
    ASSERT_EQ(OP_INPUT_MOD_NONE,  KM_MOD_NONE,  "KM_MOD_NONE");
    ASSERT_EQ(OP_INPUT_MOD_CTRL,  KM_MOD_CTRL,  "KM_MOD_CTRL");
    ASSERT_EQ(OP_INPUT_MOD_SHIFT, KM_MOD_SHIFT, "KM_MOD_SHIFT");
    ASSERT_EQ(OP_INPUT_MOD_ALT,   KM_MOD_ALT,   "KM_MOD_ALT");
    ASSERT_EQ(OP_INPUT_MOD_GUI,   KM_MOD_GUI,   "KM_MOD_GUI");
    ASSERT_EQ(OP_INPUT_CMD_KB,     KM_CMD_KB,     "KM_CMD_KB");
    ASSERT_EQ(OP_INPUT_CMD_MS_REL, KM_CMD_MS_REL, "KM_CMD_MS_REL");
    ASSERT_EQ(OP_INPUT_CMD_MS_ABS, KM_CMD_MS_ABS, "KM_CMD_MS_ABS");
}

static void test_backward_compat_sizes(void) {
    ASSERT_EQ(OP_INPUT_PKT_KEYBOARD_SIZE,  KM_PKT_KEYBOARD_SIZE,  "KM_PKT_KEYBOARD_SIZE");
    ASSERT_EQ(OP_INPUT_PKT_MOUSE_REL_SIZE, KM_PKT_MOUSE_REL_SIZE, "KM_PKT_MOUSE_REL_SIZE");
    ASSERT_EQ(OP_INPUT_PKT_MOUSE_ABS_SIZE, KM_PKT_MOUSE_ABS_SIZE, "KM_PKT_MOUSE_ABS_SIZE");
}

/* ============================================================================
 * 15. Boundary & Safety
 * ========================================================================= */

static void test_null_safety_all_apis(void) {
    op_input_hid_code_from_name(NULL);
    op_input_hid_code_from_dom_code(NULL);
    op_input_hid_code_from_char('\n', NULL);
    op_input_parse_token(NULL);
    op_input_macro_parse(NULL, NULL, 0);
    op_input_script_tokenize(NULL, NULL, 0);
}

static void test_empty_string_safety(void) {
    ASSERT_EQ(-1, op_input_hid_code_from_name(""), "empty name");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code(""), "empty dom code");
    ASSERT_EQ(-1, op_input_parse_token("").hid_code, "empty token");
    ASSERT_EQ(0, op_input_macro_parse("", NULL, 0), "empty macro");
    ASSERT_EQ(0, op_input_script_tokenize("", NULL, 0), "empty tokenize");
}

static void test_six_key_rollover_strict(void) {
    uint8_t pkt[14];
    uint8_t six[6] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, six, 6);
    for (int i = 0; i < 6; i++) {
        ASSERT_EQ(six[i], pkt[7 + i], "6-key slot filled");
    }
    uint8_t seven[7] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, seven, 7);
    ASSERT_EQ(0x09, pkt[12], "slot 6 unchanged after overflow");
}

static void test_checksum_overflow_boundary(void) {
    uint8_t data1[3] = {0x80, 0x7F, 0x00};
    ASSERT_EQ(0xFF, op_input_checksum(data1, 3), "sum=0xFF exact");
    uint8_t data2[3] = {0x80, 0x80, 0x00};
    ASSERT_EQ(0x00, op_input_checksum(data2, 3), "sum=0x100 truncates");
}

/* ============================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== Openterface Key Test Plan ===\n\n");

    printf("[1] Checksum\n");
    RUN_TEST(test_checksum_zero_data);
    RUN_TEST(test_checksum_single_byte);
    RUN_TEST(test_checksum_overflow_truncation);
    RUN_TEST(test_checksum_known_kb_packet);

    printf("\n[2] Keyboard Packet\n");
    RUN_TEST(test_kb_packet_header);
    RUN_TEST(test_kb_packet_single_key);
    RUN_TEST(test_kb_packet_combined_modifiers);
    RUN_TEST(test_kb_packet_all_modifiers);
    RUN_TEST(test_kb_packet_six_keys);
    RUN_TEST(test_kb_packet_overflow_clamped);
    RUN_TEST(test_kb_packet_zero_keys);
    RUN_TEST(test_kb_packet_negative_keys);
    RUN_TEST(test_kb_packet_checksum_filled);
    RUN_TEST(test_kb_packet_return_length);

    printf("\n[3] Press+Release\n");
    RUN_TEST(test_press_release_basic);
    RUN_TEST(test_press_release_with_modifier);
    RUN_TEST(test_press_release_both_checksums);

    printf("\n[4] ASCII -> HID\n");
    RUN_TEST(test_char_lowercase);
    RUN_TEST(test_char_uppercase);
    RUN_TEST(test_char_digits);
    RUN_TEST(test_char_shifted_symbols);
    RUN_TEST(test_char_unshifted_punctuation);
    RUN_TEST(test_char_shifted_punctuation);
    RUN_TEST(test_char_brackets);
    RUN_TEST(test_char_special_keys);
    RUN_TEST(test_char_boundary);
    RUN_TEST(test_char_out_of_range);
    RUN_TEST(test_char_null_shift_ptr);

    printf("\n[5] Named Key -> HID\n");
    RUN_TEST(test_name_all_letters);
    RUN_TEST(test_name_all_digits);
    RUN_TEST(test_name_action_keys);
    RUN_TEST(test_name_function_keys);
    RUN_TEST(test_name_arrow_keys);
    RUN_TEST(test_name_navigation_keys);
    RUN_TEST(test_name_navigation_aliases);
    RUN_TEST(test_name_modifier_keys);
    RUN_TEST(test_name_numpad);
    RUN_TEST(test_name_system_keys);
    RUN_TEST(test_name_invalid);

    printf("\n[6] HID -> Label\n");
    RUN_TEST(test_label_known_keys);
    RUN_TEST(test_label_unknown);

    printf("\n[7] DOM event.code -> HID\n");
    RUN_TEST(test_dom_letters);
    RUN_TEST(test_dom_digits);
    RUN_TEST(test_dom_punctuation);
    RUN_TEST(test_dom_action_keys);
    RUN_TEST(test_dom_navigation);
    RUN_TEST(test_dom_arrows);
    RUN_TEST(test_dom_function_keys);
    RUN_TEST(test_dom_numpad);
    RUN_TEST(test_dom_left_modifiers);
    RUN_TEST(test_dom_right_modifiers);
    RUN_TEST(test_dom_left_right_differ);
    RUN_TEST(test_dom_invalid);
    RUN_TEST(test_dom_vs_name_consistency);

    printf("\n[8] Token Parser\n");
    RUN_TEST(test_token_single_char);
    RUN_TEST(test_token_special_tags);
    RUN_TEST(test_token_arrow_tags);
    RUN_TEST(test_token_edit_tags);
    RUN_TEST(test_token_nav_tags);
    RUN_TEST(test_token_function_keys);
    RUN_TEST(test_token_modifier_tags_ignored);
    RUN_TEST(test_token_delay_tags);
    RUN_TEST(test_token_invalid);

    printf("\n[9] Macro Parser\n");
    RUN_TEST(test_macro_ctrl_c);
    RUN_TEST(test_macro_cmd_v);
    RUN_TEST(test_macro_win_r);
    RUN_TEST(test_macro_ctrl_alt_del);
    RUN_TEST(test_macro_cmd_shift_z);
    RUN_TEST(test_macro_plain_text);
    RUN_TEST(test_macro_case_insensitive);
    RUN_TEST(test_macro_shifted_symbol);
    RUN_TEST(test_macro_special_sequence);
    RUN_TEST(test_macro_delay_skipped);
    RUN_TEST(test_macro_modifier_persists);
    RUN_TEST(test_macro_null_empty);
    RUN_TEST(test_macro_max_truncation);
    RUN_TEST(test_macro_unclosed_modifier);
    RUN_TEST(test_macro_close_without_open);
    RUN_TEST(test_macro_double_modifier_idempotent);
    RUN_TEST(test_macro_complex_mixed);

    printf("\n[10] Script Tokenizer\n");
    RUN_TEST(test_tokenize_plain_text);
    RUN_TEST(test_tokenize_single_tag);
    RUN_TEST(test_tokenize_mixed);
    RUN_TEST(test_tokenize_closing_tag);
    RUN_TEST(test_tokenize_utf8_multibyte);
    RUN_TEST(test_tokenize_null);
    RUN_TEST(test_tokenize_max_truncation);
    RUN_TEST(test_tokenize_illegal_tag);

    printf("\n[11] Modifier Bitmask\n");
    RUN_TEST(test_modifier_constants);
    RUN_TEST(test_modifier_aliases);
    RUN_TEST(test_modifier_non_overlapping);

    printf("\n[12] Mouse Packets\n");
    RUN_TEST(test_mouse_rel_left_click);
    RUN_TEST(test_mouse_rel_movement);
    RUN_TEST(test_mouse_rel_scroll);
    RUN_TEST(test_mouse_rel_all_buttons);
    RUN_TEST(test_mouse_abs_coordinates);
    RUN_TEST(test_mouse_abs_header);
    RUN_TEST(test_mouse_checksum_valid);

    printf("\n[13] Hex Dump\n");
    RUN_TEST(test_hex_dump_standard);
    RUN_TEST(test_hex_dump_empty);
    RUN_TEST(test_hex_dump_single_byte);
    RUN_TEST(test_hex_dump_all_zeros);

    printf("\n[14] Backward Compat Macros\n");
    RUN_TEST(test_backward_compat_constants);
    RUN_TEST(test_backward_compat_sizes);

    printf("\n[15] Boundary & Safety\n");
    RUN_TEST(test_null_safety_all_apis);
    RUN_TEST(test_empty_string_safety);
    RUN_TEST(test_six_key_rollover_strict);
    RUN_TEST(test_checksum_overflow_boundary);

    printf("\n=== Results: %d test functions run, %d failure(s) ===\n", total, failures);
    return failures == 0 ? 0 : 1;
}
