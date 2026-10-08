#include <stdio.h>
#include <string.h>

#include "openterface/input.h"
#include "test_helpers.h"

int test_failures = 0;

/* -- Checksum ------------------------------------------------------------ */

static void test_checksum(void) {
    /* Keyboard packet with 'A' (0x04) */
    uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x57; pkt[1] = 0xAB; pkt[2] = 0x00; pkt[3] = 0x02; pkt[4] = 0x08;
    pkt[7] = 0x04; /* 'A' */
    uint8_t cs = op_input_checksum(pkt, OP_INPUT_PKT_KEYBOARD_SIZE);
    /* sum = 0x57+0xAB+0x02+0x08+0x04 = 0x110 -> 0x10 */
    ASSERT_EQ_INT(0x10, cs, "keyboard checksum");

    /* Mouse packet with dx=5 */
    uint8_t mpkt[OP_INPUT_PKT_MOUSE_REL_SIZE];
    memset(mpkt, 0, sizeof(mpkt));
    mpkt[0] = 0x57; mpkt[1] = 0xAB; mpkt[2] = 0x00; mpkt[3] = 0x05; mpkt[4] = 0x05;
    mpkt[5] = 0x01; mpkt[7] = 0x05; /* dx=5 */
    cs = op_input_checksum(mpkt, OP_INPUT_PKT_MOUSE_REL_SIZE);
    /* sum = 0x57+0xAB+0x05+0x05+0x01+0x05 = 0x112 -> 0x12 */
    ASSERT_EQ_INT(0x12, cs, "mouse checksum");
}

/* -- Keyboard packet ----------------------------------------------------─ */

static void test_keyboard_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];

    /* 'A' with no modifiers */
    uint8_t keys[6] = { 0x04, 0, 0, 0, 0, 0 };
    int len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 1, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(OP_INPUT_PKT_KEYBOARD_SIZE, len, "keyboard packet length");
    ASSERT_EQ_INT(0x57, pkt[0], "header byte 0");
    ASSERT_EQ_INT(0xAB, pkt[1], "header byte 1");
    ASSERT_EQ_INT(OP_INPUT_CMD_KB, pkt[3], "keyboard command");
    ASSERT_EQ_INT(0x08, pkt[4], "keyboard data length");
    ASSERT_EQ_INT(0x00, pkt[5], "modifiers");
    ASSERT_EQ_INT(0x04, pkt[7], "key slot 1 = A");
    ASSERT_EQ_INT(0x00, pkt[8], "key slot 2 empty");

    /* Ctrl+C */
    keys[0] = 0x06;
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL, keys, 1, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(0x01, pkt[5], "Ctrl modifier");
    ASSERT_EQ_INT(0x06, pkt[7], "key slot 1 = C");

    /* Multi-key: Ctrl+Shift+A+B */
    uint8_t multi[6] = { 0x04, 0x05, 0, 0, 0, 0 };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL | OP_INPUT_MOD_SHIFT, multi, 2, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(0x03, pkt[5], "Ctrl|Shift modifiers");
    ASSERT_EQ_INT(0x04, pkt[7], "key A");
    ASSERT_EQ_INT(0x05, pkt[8], "key B");
    ASSERT_EQ_INT(0x00, pkt[9], "key slot 3 empty");

    /* Exactly 6 keys fills all slots */
    uint8_t six[6] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, six, 6, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(0x04, pkt[7],  "slot 1");
    ASSERT_EQ_INT(0x09, pkt[12], "slot 6 (last key slot)");

    /* More than 6 keys — extras silently dropped */
    uint8_t seven[7] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, seven, 7, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(0x09, pkt[12], "7th key dropped, slot 6 unchanged");

    /* Verify checksum is non-zero (actual value depends on packet content) */
    ASSERT_EQ_INT(pkt[13], op_input_checksum(pkt, OP_INPUT_PKT_KEYBOARD_SIZE), "checksum matches");

    /* Verify checksum changes when packet changes */
    uint8_t saved = pkt[13];
    pkt[7] ^= 0xFF;  /* flip a byte */
    ASSERT_EQ_INT(saved != op_input_checksum(pkt, OP_INPUT_PKT_KEYBOARD_SIZE), 1, "checksum changes with data");
}

/* -- Mouse packet -------------------------------------------------------- */

static void test_mouse_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_MOUSE_REL_SIZE];

    /* Left click */
    int len = op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT, 0, 0, 0);
    ASSERT_EQ_INT(OP_INPUT_PKT_MOUSE_REL_SIZE, len, "mouse packet length");
    ASSERT_EQ_INT(0x57, pkt[0], "header byte 0");
    ASSERT_EQ_INT(OP_INPUT_CMD_MS_REL, pkt[3], "mouse command");
    ASSERT_EQ_INT(0x05, pkt[4], "mouse data length");
    ASSERT_EQ_INT(0x01, pkt[5], "relative mode");
    ASSERT_EQ_INT(OP_INPUT_MS_BTN_LEFT, pkt[6], "left button");
    ASSERT_EQ_INT(0x00, pkt[7], "dx = 0");
    ASSERT_EQ_INT(0x00, pkt[8], "dy = 0");
    ASSERT_EQ_INT(0x00, pkt[9], "wheel = 0");

    /* Movement with deltas */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 50, -30, 0);
    ASSERT_EQ_INT(50, (int8_t)pkt[7], "dx = 50");
    ASSERT_EQ_INT(-30, (int8_t)pkt[8], "dy = -30");

    /* Scroll */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 3);
    ASSERT_EQ_INT(3, pkt[9], "wheel = 3");

    /* Scroll down (negative wraps) */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, -5);
    ASSERT_EQ_INT(-5, (int8_t)pkt[9], "wheel = -5");

    /* All buttons */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT | OP_INPUT_MS_BTN_RIGHT | OP_INPUT_MS_BTN_MIDDLE, 0, 0, 0);
    ASSERT_EQ_INT(0x07, pkt[6], "all buttons bitmask");
}

static void test_mouse_abs_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_MOUSE_ABS_SIZE];

    /* Absolute packet: 57 AB 00 04 07 02 [buttons] [xL] [xH] [yL] [yH] [wheel] [sum] */
    int len = op_input_build_mouse_abs(pkt, OP_INPUT_MS_BTN_NONE, 0x0123, 0x0456, -1);
    ASSERT_EQ_INT(OP_INPUT_PKT_MOUSE_ABS_SIZE, len, "mouse abs packet length");
    ASSERT_EQ_INT(0x57, pkt[0], "abs header byte 0");
    ASSERT_EQ_INT(0xAB, pkt[1], "abs header byte 1");
    ASSERT_EQ_INT(OP_INPUT_CMD_MS_ABS, pkt[3], "abs mouse command");
    ASSERT_EQ_INT(0x07, pkt[4], "abs mouse data length");
    ASSERT_EQ_INT(0x02, pkt[5], "absolute mode");
    ASSERT_EQ_INT(0x00, pkt[6], "abs buttons = none");
    ASSERT_EQ_INT(0x23, pkt[7], "abs x low");
    ASSERT_EQ_INT(0x01, pkt[8], "abs x high");
    ASSERT_EQ_INT(0x56, pkt[9], "abs y low");
    ASSERT_EQ_INT(0x04, pkt[10], "abs y high");
    ASSERT_EQ_INT(-1, (int8_t)pkt[11], "abs wheel = -1");

    /* Verify checksum is non-zero (actual value depends on packet content) */
    ASSERT_EQ_INT(pkt[12], op_input_checksum(pkt, OP_INPUT_PKT_MOUSE_ABS_SIZE), "checksum matches");

    /* Verify checksum changes when packet changes */
    uint8_t saved = pkt[12];
    pkt[7] ^= 0xFF;  /* flip a byte */
    ASSERT_EQ_INT(saved != op_input_checksum(pkt, OP_INPUT_PKT_MOUSE_ABS_SIZE), 1, "checksum changes with data");
}

/* -- Press + release ----------------------------------------------------─ */

static void test_press_release(void) {
    uint8_t out[2 * OP_INPUT_PKT_KEYBOARD_SIZE];
    int len = op_input_build_press_release(out, OP_INPUT_MOD_NONE, 0x28 /* Enter */, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(2 * OP_INPUT_PKT_KEYBOARD_SIZE, len, "press+release length");

    /* Press */
    ASSERT_EQ_INT(0x28, out[7], "press key = Enter");
    /* Release */
    ASSERT_EQ_INT(0x00, out[OP_INPUT_PKT_KEYBOARD_SIZE + 5], "release modifiers = 0");
    ASSERT_EQ_INT(0x00, out[OP_INPUT_PKT_KEYBOARD_SIZE + 7], "release key slot 1 = 0");
}

/* -- HID code lookup ----------------------------------------------------─ */

static void test_hid_codes(void) {
    /* Letters */
    ASSERT_EQ_INT(0x04, op_input_hid_code_from_name("A"), "A");
    ASSERT_EQ_INT(0x1D, op_input_hid_code_from_name("Z"), "Z");
    ASSERT_EQ_INT(0x04, op_input_hid_code_from_name("a"), "a");

    /* Numbers */
    ASSERT_EQ_INT(0x1E, op_input_hid_code_from_name("1"), "1");
    ASSERT_EQ_INT(0x27, op_input_hid_code_from_name("0"), "0");

    /* Special keys */
    ASSERT_EQ_INT(0x28, op_input_hid_code_from_name("Enter"), "Enter");
    ASSERT_EQ_INT(0x29, op_input_hid_code_from_name("Escape"), "Escape");
    ASSERT_EQ_INT(0x2A, op_input_hid_code_from_name("Backspace"), "Backspace");
    ASSERT_EQ_INT(0x2B, op_input_hid_code_from_name("Tab"), "Tab");
    ASSERT_EQ_INT(0x2C, op_input_hid_code_from_name("Space"), "Space");

    /* Function keys */
    ASSERT_EQ_INT(0x3A, op_input_hid_code_from_name("F1"), "F1");
    ASSERT_EQ_INT(0x45, op_input_hid_code_from_name("F12"), "F12");

    /* Arrows */
    ASSERT_EQ_INT(0x52, op_input_hid_code_from_name("Up"), "Up");
    ASSERT_EQ_INT(0x51, op_input_hid_code_from_name("Down"), "Down");
    ASSERT_EQ_INT(0x50, op_input_hid_code_from_name("Left"), "Left");
    ASSERT_EQ_INT(0x4F, op_input_hid_code_from_name("Right"), "Right");

    /* Modifiers */
    ASSERT_EQ_INT(0xE0, op_input_hid_code_from_name("Ctrl"), "Ctrl");
    ASSERT_EQ_INT(0xE1, op_input_hid_code_from_name("Shift"), "Shift");
    ASSERT_EQ_INT(0xE2, op_input_hid_code_from_name("Alt"), "Alt");
    ASSERT_EQ_INT(0xE3, op_input_hid_code_from_name("Cmd"), "Cmd");
    ASSERT_EQ_INT(0xE3, op_input_hid_code_from_name("Win"), "Win");

    /* Numpad */
    ASSERT_EQ_INT(0x62, op_input_hid_code_from_name("Numpad0"), "Numpad0");
    ASSERT_EQ_INT(0x54, op_input_hid_code_from_name("NumpadSlash"), "NumpadSlash");

    /* Unknown */
    ASSERT_EQ_INT(-1, op_input_hid_code_from_name("NoSuchKey"), "unknown key");
    ASSERT_EQ_INT(-1, op_input_hid_code_from_name(""), "empty key");
    ASSERT_EQ_INT(-1, op_input_hid_code_from_name(NULL), "NULL key");

    /* Label lookup */
    ASSERT_EQ_STR("Enter", op_input_hid_code_label(0x28), "label Enter");
    ASSERT_EQ_STR("A", op_input_hid_code_label(0x04), "label A");
    ASSERT_EQ_STR("Unknown", op_input_hid_code_label(0xFF), "unknown label");
}

/* -- Char → HID ---------------------------------------------------------- */

static void test_char_to_hid(void) {
    int shift;

    ASSERT_EQ_INT(0x04, op_input_hid_code_from_char('a', &shift), "'a' HID");
    ASSERT_EQ_INT(0, shift, "'a' no shift");

    ASSERT_EQ_INT(0x04, op_input_hid_code_from_char('A', &shift), "'A' HID");
    ASSERT_EQ_INT(1, shift, "'A' needs shift");

    ASSERT_EQ_INT(0x1E, op_input_hid_code_from_char('1', &shift), "'1' HID");
    ASSERT_EQ_INT(0, shift, "'1' no shift");

    ASSERT_EQ_INT(0x1E, op_input_hid_code_from_char('!', &shift), "'!' HID");
    ASSERT_EQ_INT(1, shift, "'!' needs shift");

    ASSERT_EQ_INT(0x2C, op_input_hid_code_from_char(' ', &shift), "space HID");
    ASSERT_EQ_INT(0, shift, "space no shift");

    ASSERT_EQ_INT(-1, op_input_hid_code_from_char('\n', &shift), "newline unmapped");
    ASSERT_EQ_INT(-1, op_input_hid_code_from_char('\x7F', &shift), "DEL unmapped");
}

/* -- Token parser -------------------------------------------------------- */

static void test_token_parser(void) {
    op_input_parsed_token_t t;

    /* Single character */
    t = op_input_parse_token("A");
    ASSERT_EQ_INT(0x04, t.hid_code, "token 'A'");
    ASSERT_EQ_INT(0, t.modifiers, "token 'A' no modifier");

    /* Special token */
    t = op_input_parse_token("<ENTER>");
    ASSERT_EQ_INT(0x28, t.hid_code, "token <ENTER>");

    /* Function key */
    t = op_input_parse_token("<F5>");
    ASSERT_EQ_INT(0x3E, t.hid_code, "token <F5>");

    /* Modifier opening — no key, no modifier on result */
    t = op_input_parse_token("<CTRL>");
    ASSERT_EQ_INT(-1, t.hid_code, "modifier has no key");
    ASSERT_EQ_INT(0, t.modifiers, "modifier result has no modifier flag");

    /* Delay token — ignored */
    t = op_input_parse_token("<DELAY1S>");
    ASSERT_EQ_INT(-1, t.hid_code, "delay ignored");

    /* Unknown */
    t = op_input_parse_token("???");
    ASSERT_EQ_INT(-1, t.hid_code, "unknown token");
}

/* -- Macro parser -------------------------------------------------------- */

static void test_macro_parser(void) {
    op_input_parsed_token_t tokens[32];
    int n;

    /* Ctrl+C — modifier tags don't emit tokens, only the key */
    n = op_input_macro_parse("<CTRL>C</CTRL>", tokens, 32);
    ASSERT_EQ_INT(1, n, "Ctrl+C token count");
    ASSERT_EQ_INT(0x06, tokens[0].hid_code, "C key code");
    ASSERT_EQ_INT(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl modifier");

    /* Cmd+V */
    n = op_input_macro_parse("<CMD>V</CMD>", tokens, 32);
    ASSERT_EQ_INT(1, n, "Cmd+V token count");
    ASSERT_EQ_INT(0x19, tokens[0].hid_code, "V key code");
    ASSERT_EQ_INT(OP_INPUT_MOD_GUI, tokens[0].modifiers, "GUI modifier");

    /* Ctrl+Alt+Del — only <DEL> emits (bare "DEL" is not a special token) */
    n = op_input_macro_parse("<CTRL><ALT><DEL></ALT></CTRL>", tokens, 32);
    ASSERT_EQ_INT(1, n, "Ctrl+Alt+Del token count");
    ASSERT_EQ_INT(0x4C, tokens[0].hid_code, "Del key code");
    ASSERT_EQ_INT(OP_INPUT_MOD_CTRL | OP_INPUT_MOD_ALT, tokens[0].modifiers, "Ctrl|Alt modifiers");

    /* Plain text */
    n = op_input_macro_parse("abc", tokens, 32);
    ASSERT_EQ_INT(3, n, "'abc' token count");
    ASSERT_EQ_INT(0x04, tokens[0].hid_code, "a");
    ASSERT_EQ_INT(0x05, tokens[1].hid_code, "b");
    ASSERT_EQ_INT(0x06, tokens[2].hid_code, "c");

    /* Text with shift symbols — letters are case-insensitive in macros */
    n = op_input_macro_parse("Hi!", tokens, 32);
    ASSERT_EQ_INT(3, n, "'Hi!' token count");
    ASSERT_EQ_INT(0, tokens[0].modifiers & OP_INPUT_MOD_SHIFT, "'H' (lowercased) no shift");
    ASSERT_NEQ(0, tokens[2].modifiers & OP_INPUT_MOD_SHIFT, "'!' has shift");

    /* Mixed macro with special keys */
    n = op_input_macro_parse("<F1><ENTER>", tokens, 32);
    ASSERT_EQ_INT(2, n, "F1+Enter count");
    ASSERT_EQ_INT(0x3A, tokens[0].hid_code, "F1");
    ASSERT_EQ_INT(0x28, tokens[1].hid_code, "Enter");

    /* Delay tokens are skipped */
    n = op_input_macro_parse("<DELAY1S><CTRL>A</CTRL>", tokens, 32);
    ASSERT_EQ_INT(1, n, "delay + Ctrl+A count");
    ASSERT_EQ_INT(0x04, tokens[0].hid_code, "A after delay");
    ASSERT_EQ_INT(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl modifier preserved");

    /* NULL / empty input */
    n = op_input_macro_parse(NULL, tokens, 32);
    ASSERT_EQ_INT(0, n, "NULL input");
    n = op_input_macro_parse("", tokens, 32);
    ASSERT_EQ_INT(0, n, "empty input");

    /* Max limit */
    n = op_input_macro_parse("abcdefghij", tokens, 3);
    ASSERT_EQ_INT(3, n, "max limit respected");

    /* Modifier state persists across keys */
    n = op_input_macro_parse("<CMD>AB</CMD>", tokens, 32);
    ASSERT_EQ_INT(2, n, "Cmd+AB count");
    ASSERT_EQ_INT(OP_INPUT_MOD_GUI, tokens[0].modifiers, "A has GUI");
    ASSERT_EQ_INT(OP_INPUT_MOD_GUI, tokens[1].modifiers, "B has GUI");

    /* Cmd+Shift+Z (redo) — macro parser lowercases letters, so Z → 'z' = 0x1D */
    n = op_input_macro_parse("<CMD><SHIFT>Z</SHIFT></CMD>", tokens, 32);
    ASSERT_EQ_INT(1, n, "Cmd+Shift+Z count");
    ASSERT_EQ_INT(0x1D, tokens[0].hid_code, "Z key code (lowercase)");
    ASSERT_EQ_INT(OP_INPUT_MOD_GUI | OP_INPUT_MOD_SHIFT, tokens[0].modifiers, "GUI|Shift modifiers");
}

/* -- CH9329 workaround --------------------------------------------------- */

static void test_keyboard_workaround(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* LCtrl + LAlt + 'A' with workaround */
    int len = op_input_build_keyboard(buf,
        OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_LALT,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");

    /* Header */
    ASSERT_EQ_INT(0x57, buf[0], "header byte 0");
    ASSERT_EQ_INT(0xAB, buf[1], "header byte 1");
    ASSERT_EQ_INT(0x02, buf[3], "keyboard command");

    /* Modifier byte: only Ctrl (bit 0), Alt (bit 2) masked out */
    ASSERT_EQ_INT(0x01, buf[5], "only LCtrl in modifier byte");

    /* Key array: LCtrl (0xE0) + LAlt (0xE2) + 'A' (0x04) */
    ASSERT_EQ_INT(0xE0, buf[7], "LCtrl HID code");
    ASSERT_EQ_INT(0xE2, buf[8], "LAlt HID code");
    ASSERT_EQ_INT(0x04, buf[9], "'A' key");
    ASSERT_EQ_INT(0x00, buf[10], "slot 4 empty");
    ASSERT_EQ_INT(0x00, buf[11], "slot 5 empty");
    ASSERT_EQ_INT(0x00, buf[12], "slot 6 empty");
}

static void test_keyboard_workaround_right_modifiers(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* RCtrl + RShift + 'A' with workaround — right-side Ctrl/Shift should be preserved */
    int len = op_input_build_keyboard(buf,
        OP_INPUT_MOD_RCTRL | OP_INPUT_MOD_RSHIFT,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");

    /* Modifier byte: RCtrl (bit 4) + RShift (bit 5) = 0x30 */
    ASSERT_EQ_INT(0x30, buf[5], "RCtrl+RShift preserved in modifier byte");

    /* Key array: RCtrl (0xE4) + RShift (0xE5) + 'A' (0x04) */
    ASSERT_EQ_INT(0xE4, buf[7], "RCtrl HID code");
    ASSERT_EQ_INT(0xE5, buf[8], "RShift HID code");
    ASSERT_EQ_INT(0x04, buf[9], "'A' key");
}

static void test_keyboard_workaround_only_alt_gui(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* LAlt + LGUI + 'A' — core workaround scenario: modifier byte should be 0 */
    int len = op_input_build_keyboard(buf,
        OP_INPUT_MOD_LALT | OP_INPUT_MOD_LGUI,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");

    /* Modifier byte: Alt/GUI masked out */
    ASSERT_EQ_INT(0x00, buf[5], "modifier byte = 0 for Alt/GUI only");

    /* Key array: LAlt (0xE2) + LGUI (0xE3) + 'A' (0x04) */
    ASSERT_EQ_INT(0xE2, buf[7], "LAlt HID code");
    ASSERT_EQ_INT(0xE3, buf[8], "LGUI HID code");
    ASSERT_EQ_INT(0x04, buf[9], "'A' key");
}

static void test_keyboard_workaround_zero_modifiers(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* No modifiers — should behave same as standard mode */
    int len = op_input_build_keyboard(buf,
        OP_INPUT_MOD_NONE,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");
    ASSERT_EQ_INT(0x00, buf[5], "modifier byte = 0");
    ASSERT_EQ_INT(0x04, buf[7], "'A' key in slot 1");
    ASSERT_EQ_INT(0x00, buf[8], "slot 2 empty");
}

static void test_keyboard_workaround_mixed_modifiers(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* LCtrl + RShift + LAlt + 'A' — mixed left/right modifiers */
    int len = op_input_build_keyboard(buf,
        OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_RSHIFT | OP_INPUT_MOD_LALT,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");

    /* Modifier byte: LCtrl (bit 0) + RShift (bit 5) = 0x21, Alt masked out */
    ASSERT_EQ_INT(0x21, buf[5], "LCtrl+RShift preserved, Alt masked");

    /* Key array: modifiers in bit order — LCtrl (0xE0) + LAlt (0xE2) + RShift (0xE5) + 'A' (0x04) */
    ASSERT_EQ_INT(0xE0, buf[7], "LCtrl HID code");
    ASSERT_EQ_INT(0xE2, buf[8], "LAlt HID code");
    ASSERT_EQ_INT(0xE5, buf[9], "RShift HID code");
    ASSERT_EQ_INT(0x04, buf[10], "'A' key");
}

static void test_keyboard_workaround_modifier_overflow(void) {
    uint8_t buf[OP_INPUT_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = { 0x04 }; /* 'A' key */

    /* All 8 modifiers + 'A' — overflow: only 6 slots in key array */
    uint8_t all_mods = OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_LSHIFT | OP_INPUT_MOD_LALT | OP_INPUT_MOD_LGUI
                     | OP_INPUT_MOD_RCTRL | OP_INPUT_MOD_RSHIFT | OP_INPUT_MOD_RALT | OP_INPUT_MOD_RGUI;
    int len = op_input_build_keyboard(buf,
        all_mods,
        keys, 1,
        OP_INPUT_KB_FLAG_CH9329_WORKAROUND);

    ASSERT_EQ_INT(len, OP_INPUT_PKT_KEYBOARD_SIZE, "workaround packet length");

    /* Modifier byte: Ctrl/Shift preserved (0x33), Alt/GUI masked */
    ASSERT_EQ_INT(0x33, buf[5], "Ctrl/Shift preserved in modifier byte");

    /* Key array: 6 modifier HID codes, 'A' dropped due to overflow */
    ASSERT_EQ_INT(0xE0, buf[7], "LCtrl HID code");
    ASSERT_EQ_INT(0xE1, buf[8], "LShift HID code");
    ASSERT_EQ_INT(0xE2, buf[9], "LAlt HID code");
    ASSERT_EQ_INT(0xE3, buf[10], "LGUI HID code");
    ASSERT_EQ_INT(0xE4, buf[11], "RCtrl HID code");
    ASSERT_EQ_INT(0xE5, buf[12], "RShift HID code");
    /* RAlt (0xE6), RGUI (0xE7), and 'A' (0x04) are dropped */
}

/* -- Hex dump ------------------------------------------------------------ */

static void test_hex_dump(void) {
    uint8_t data[] = { 0x57, 0xAB, 0x00, 0x02 };
    char buf[32];
    op_input_hex_dump(data, 4, buf);
    ASSERT_EQ_STR("57AB0002", buf, "hex dump");

    op_input_hex_dump(data, 0, buf);
    ASSERT_EQ_STR("", buf, "hex dump empty");
}

/* -- Main ---------------------------------------------------------------- */

int main(void) {
    printf("Running input tests...\n");

    RUN_TEST(test_checksum);
    RUN_TEST(test_keyboard_packet);
    RUN_TEST(test_mouse_packet);
    RUN_TEST(test_mouse_abs_packet);
    RUN_TEST(test_press_release);
    RUN_TEST(test_hid_codes);
    RUN_TEST(test_char_to_hid);
    RUN_TEST(test_token_parser);
    RUN_TEST(test_macro_parser);
    RUN_TEST(test_keyboard_workaround);
    RUN_TEST(test_keyboard_workaround_right_modifiers);
    RUN_TEST(test_keyboard_workaround_only_alt_gui);
    RUN_TEST(test_keyboard_workaround_zero_modifiers);
    RUN_TEST(test_keyboard_workaround_mixed_modifiers);
    RUN_TEST(test_keyboard_workaround_modifier_overflow);
    RUN_TEST(test_hex_dump);

    if (test_failures != 0) {
        fprintf(stderr, "input_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("input_test: all tests passed\n");
    return 0;
}
