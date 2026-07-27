#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "openterface/input.h"

static int failures = 0;

#define ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        fprintf(stderr, "FAIL: %s: expected %d (0x%02X), got %d (0x%02X)\n", \
                msg, (int)(expected), (int)(expected), (int)(actual), (int)(actual)); \
        failures++; \
    } \
} while(0)

#define ASSERT_NEQ(bad, actual, msg) do { \
    if ((bad) == (actual)) { \
        fprintf(stderr, "FAIL: %s: expected != %d (0x%02X)\n", \
                msg, (int)(bad), (int)(bad)); \
        failures++; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, "FAIL: %s: expected '%s', got '%s'\n", msg, expected, actual); \
        failures++; \
    } \
} while(0)

/* ── Checksum ──────────────────────────────────────────────────────────── */

static void test_checksum(void) {
    /* Keyboard packet with 'A' (0x04) */
    uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x57; pkt[1] = 0xAB; pkt[2] = 0x00; pkt[3] = 0x02; pkt[4] = 0x08;
    pkt[7] = 0x04; /* 'A' */
    uint8_t cs = op_input_checksum(pkt, OP_INPUT_PKT_KEYBOARD_SIZE);
    /* sum = 0x57+0xAB+0x02+0x08+0x04 = 0x110 -> 0x10 */
    ASSERT_EQ(0x10, cs, "keyboard checksum");

    /* Mouse packet with dx=5 */
    uint8_t mpkt[OP_INPUT_PKT_MOUSE_REL_SIZE];
    memset(mpkt, 0, sizeof(mpkt));
    mpkt[0] = 0x57; mpkt[1] = 0xAB; mpkt[2] = 0x00; mpkt[3] = 0x05; mpkt[4] = 0x05;
    mpkt[5] = 0x01; mpkt[7] = 0x05; /* dx=5 */
    cs = op_input_checksum(mpkt, OP_INPUT_PKT_MOUSE_REL_SIZE);
    /* sum = 0x57+0xAB+0x05+0x05+0x01+0x05 = 0x112 -> 0x12 */
    ASSERT_EQ(0x12, cs, "mouse checksum");
}

/* ── Keyboard packet ───────────────────────────────────────────────────── */

static void test_keyboard_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];

    /* 'A' with no modifiers */
    uint8_t keys[6] = { 0x04, 0, 0, 0, 0, 0 };
    int len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, keys, 1);
    ASSERT_EQ(OP_INPUT_PKT_KEYBOARD_SIZE, len, "keyboard packet length");
    ASSERT_EQ(0x57, pkt[0], "header byte 0");
    ASSERT_EQ(0xAB, pkt[1], "header byte 1");
    ASSERT_EQ(OP_INPUT_CMD_KB, pkt[3], "keyboard command");
    ASSERT_EQ(0x08, pkt[4], "keyboard data length");
    ASSERT_EQ(0x00, pkt[5], "modifiers");
    ASSERT_EQ(0x04, pkt[7], "key slot 1 = A");
    ASSERT_EQ(0x00, pkt[8], "key slot 2 empty");

    /* Ctrl+C */
    keys[0] = 0x06;
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL, keys, 1);
    ASSERT_EQ(0x01, pkt[5], "Ctrl modifier");
    ASSERT_EQ(0x06, pkt[7], "key slot 1 = C");

    /* Multi-key: Ctrl+Shift+A+B */
    uint8_t multi[6] = { 0x04, 0x05, 0, 0, 0, 0 };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL | OP_INPUT_MOD_SHIFT, multi, 2);
    ASSERT_EQ(0x03, pkt[5], "Ctrl|Shift modifiers");
    ASSERT_EQ(0x04, pkt[7], "key A");
    ASSERT_EQ(0x05, pkt[8], "key B");
    ASSERT_EQ(0x00, pkt[9], "key slot 3 empty");

    /* Exactly 6 keys fills all slots */
    uint8_t six[6] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, six, 6);
    ASSERT_EQ(0x04, pkt[7],  "slot 1");
    ASSERT_EQ(0x09, pkt[12], "slot 6 (last key slot)");

    /* More than 6 keys — extras silently dropped */
    uint8_t seven[7] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    len = op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, seven, 7);
    ASSERT_EQ(0x09, pkt[12], "7th key dropped, slot 6 unchanged");

    /* Verify checksum is filled */
    uint8_t expected_cs = op_input_checksum(pkt, OP_INPUT_PKT_KEYBOARD_SIZE);
    ASSERT_EQ(expected_cs, pkt[13], "checksum byte");
}

/* ── Mouse packet ──────────────────────────────────────────────────────── */

static void test_mouse_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_MOUSE_REL_SIZE];

    /* Left click */
    int len = op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT, 0, 0, 0);
    ASSERT_EQ(OP_INPUT_PKT_MOUSE_REL_SIZE, len, "mouse packet length");
    ASSERT_EQ(0x57, pkt[0], "header byte 0");
    ASSERT_EQ(OP_INPUT_CMD_MS_REL, pkt[3], "mouse command");
    ASSERT_EQ(0x05, pkt[4], "mouse data length");
    ASSERT_EQ(0x01, pkt[5], "relative mode");
    ASSERT_EQ(OP_INPUT_MS_BTN_LEFT, pkt[6], "left button");
    ASSERT_EQ(0x00, pkt[7], "dx = 0");
    ASSERT_EQ(0x00, pkt[8], "dy = 0");
    ASSERT_EQ(0x00, pkt[9], "wheel = 0");

    /* Movement with deltas */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 50, -30, 0);
    ASSERT_EQ(50, (int8_t)pkt[7], "dx = 50");
    ASSERT_EQ(-30, (int8_t)pkt[8], "dy = -30");

    /* Scroll */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 3);
    ASSERT_EQ(3, pkt[9], "wheel = 3");

    /* Scroll down (negative wraps) */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, -5);
    ASSERT_EQ(-5, (int8_t)pkt[9], "wheel = -5");

    /* All buttons */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT | OP_INPUT_MS_BTN_RIGHT | OP_INPUT_MS_BTN_MIDDLE, 0, 0, 0);
    ASSERT_EQ(0x07, pkt[6], "all buttons bitmask");
}

static void test_mouse_abs_packet(void) {
    uint8_t pkt[OP_INPUT_PKT_MOUSE_ABS_SIZE];

    /* Absolute packet: 57 AB 00 04 07 02 [buttons] [xL] [xH] [yL] [yH] [wheel] [sum] */
    int len = op_input_build_mouse_abs(pkt, OP_INPUT_MS_BTN_NONE, 0x0123, 0x0456, -1);
    ASSERT_EQ(OP_INPUT_PKT_MOUSE_ABS_SIZE, len, "mouse abs packet length");
    ASSERT_EQ(0x57, pkt[0], "abs header byte 0");
    ASSERT_EQ(0xAB, pkt[1], "abs header byte 1");
    ASSERT_EQ(OP_INPUT_CMD_MS_ABS, pkt[3], "abs mouse command");
    ASSERT_EQ(0x07, pkt[4], "abs mouse data length");
    ASSERT_EQ(0x02, pkt[5], "absolute mode");
    ASSERT_EQ(0x00, pkt[6], "abs buttons = none");
    ASSERT_EQ(0x23, pkt[7], "abs x low");
    ASSERT_EQ(0x01, pkt[8], "abs x high");
    ASSERT_EQ(0x56, pkt[9], "abs y low");
    ASSERT_EQ(0x04, pkt[10], "abs y high");
    ASSERT_EQ(-1, (int8_t)pkt[11], "abs wheel = -1");

    uint8_t expected_cs = op_input_checksum(pkt, OP_INPUT_PKT_MOUSE_ABS_SIZE);
    ASSERT_EQ(expected_cs, pkt[12], "abs checksum byte");
}

/* ── Press + release ───────────────────────────────────────────────────── */

static void test_press_release(void) {
    uint8_t out[2 * OP_INPUT_PKT_KEYBOARD_SIZE];
    int len = op_input_build_press_release(out, OP_INPUT_MOD_NONE, 0x28 /* Enter */);
    ASSERT_EQ(2 * OP_INPUT_PKT_KEYBOARD_SIZE, len, "press+release length");

    /* Press */
    ASSERT_EQ(0x28, out[7], "press key = Enter");
    /* Release */
    ASSERT_EQ(0x00, out[OP_INPUT_PKT_KEYBOARD_SIZE + 5], "release modifiers = 0");
    ASSERT_EQ(0x00, out[OP_INPUT_PKT_KEYBOARD_SIZE + 7], "release key slot 1 = 0");
}

/* ── DOM event code → HID ──────────────────────────────────────────────── */

static void test_dom_code_mapping(void) {
    /* Letters */
    ASSERT_EQ(0x04, op_input_hid_code_from_dom_code("KeyA"), "KeyA");
    ASSERT_EQ(0x1D, op_input_hid_code_from_dom_code("KeyZ"), "KeyZ");
    /* Digits */
    ASSERT_EQ(0x1E, op_input_hid_code_from_dom_code("Digit1"), "Digit1");
    ASSERT_EQ(0x27, op_input_hid_code_from_dom_code("Digit0"), "Digit0");
    /* Modifiers */
    ASSERT_EQ(0xE0, op_input_hid_code_from_dom_code("ControlLeft"), "ControlLeft");
    ASSERT_EQ(0xE4, op_input_hid_code_from_dom_code("ControlRight"), "ControlRight");
    ASSERT_EQ(0xE1, op_input_hid_code_from_dom_code("ShiftLeft"), "ShiftLeft");
    ASSERT_EQ(0xE3, op_input_hid_code_from_dom_code("MetaLeft"), "MetaLeft");
    /* Arrows */
    ASSERT_EQ(0x52, op_input_hid_code_from_dom_code("ArrowUp"), "ArrowUp");
    /* Punctuation */
    ASSERT_EQ(0x2D, op_input_hid_code_from_dom_code("Minus"), "Minus");
    ASSERT_EQ(0x2F, op_input_hid_code_from_dom_code("BracketLeft"), "BracketLeft");
    /* Numpad */
    ASSERT_EQ(0x59, op_input_hid_code_from_dom_code("Numpad1"), "Numpad1");
    /* Unknown / edge cases */
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code("BrowserBack"), "unknown");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code(""), "empty");
    ASSERT_EQ(-1, op_input_hid_code_from_dom_code(NULL), "NULL");
}

/* ── HID code lookup ───────────────────────────────────────────────────── */

static void test_hid_codes(void) {
    /* Letters */
    ASSERT_EQ(0x04, op_input_hid_code_from_name("A"), "A");
    ASSERT_EQ(0x1D, op_input_hid_code_from_name("Z"), "Z");
    ASSERT_EQ(0x04, op_input_hid_code_from_name("a"), "a");

    /* Numbers */
    ASSERT_EQ(0x1E, op_input_hid_code_from_name("1"), "1");
    ASSERT_EQ(0x27, op_input_hid_code_from_name("0"), "0");

    /* Special keys */
    ASSERT_EQ(0x28, op_input_hid_code_from_name("Enter"), "Enter");
    ASSERT_EQ(0x29, op_input_hid_code_from_name("Escape"), "Escape");
    ASSERT_EQ(0x2A, op_input_hid_code_from_name("Backspace"), "Backspace");
    ASSERT_EQ(0x2B, op_input_hid_code_from_name("Tab"), "Tab");
    ASSERT_EQ(0x2C, op_input_hid_code_from_name("Space"), "Space");

    /* Function keys */
    ASSERT_EQ(0x3A, op_input_hid_code_from_name("F1"), "F1");
    ASSERT_EQ(0x45, op_input_hid_code_from_name("F12"), "F12");

    /* Arrows */
    ASSERT_EQ(0x52, op_input_hid_code_from_name("Up"), "Up");
    ASSERT_EQ(0x51, op_input_hid_code_from_name("Down"), "Down");
    ASSERT_EQ(0x50, op_input_hid_code_from_name("Left"), "Left");
    ASSERT_EQ(0x4F, op_input_hid_code_from_name("Right"), "Right");

    /* Modifiers */
    ASSERT_EQ(0xE0, op_input_hid_code_from_name("Ctrl"), "Ctrl");
    ASSERT_EQ(0xE1, op_input_hid_code_from_name("Shift"), "Shift");
    ASSERT_EQ(0xE2, op_input_hid_code_from_name("Alt"), "Alt");
    ASSERT_EQ(0xE3, op_input_hid_code_from_name("Cmd"), "Cmd");
    ASSERT_EQ(0xE3, op_input_hid_code_from_name("Win"), "Win");

    /* Numpad */
    ASSERT_EQ(0x62, op_input_hid_code_from_name("Numpad0"), "Numpad0");
    ASSERT_EQ(0x54, op_input_hid_code_from_name("NumpadSlash"), "NumpadSlash");

    /* Unknown */
    ASSERT_EQ(-1, op_input_hid_code_from_name("NoSuchKey"), "unknown key");
    ASSERT_EQ(-1, op_input_hid_code_from_name(""), "empty key");
    ASSERT_EQ(-1, op_input_hid_code_from_name(NULL), "NULL key");

    /* Label lookup */
    ASSERT_STR_EQ("Enter", op_input_hid_code_label(0x28), "label Enter");
    ASSERT_STR_EQ("A", op_input_hid_code_label(0x04), "label A");
    ASSERT_STR_EQ("Unknown", op_input_hid_code_label(0xFF), "unknown label");
}

/* ── Char → HID ────────────────────────────────────────────────────────── */

static void test_char_to_hid(void) {
    int shift;

    ASSERT_EQ(0x04, op_input_hid_code_from_char('a', &shift), "'a' HID");
    ASSERT_EQ(0, shift, "'a' no shift");

    ASSERT_EQ(0x04, op_input_hid_code_from_char('A', &shift), "'A' HID");
    ASSERT_EQ(1, shift, "'A' needs shift");

    ASSERT_EQ(0x1E, op_input_hid_code_from_char('1', &shift), "'1' HID");
    ASSERT_EQ(0, shift, "'1' no shift");

    ASSERT_EQ(0x1E, op_input_hid_code_from_char('!', &shift), "'!' HID");
    ASSERT_EQ(1, shift, "'!' needs shift");

    ASSERT_EQ(0x2C, op_input_hid_code_from_char(' ', &shift), "space HID");
    ASSERT_EQ(0, shift, "space no shift");

    ASSERT_EQ(-1, op_input_hid_code_from_char('\n', &shift), "newline unmapped");
    ASSERT_EQ(-1, op_input_hid_code_from_char('\x7F', &shift), "DEL unmapped");
}

/* ── Token parser ──────────────────────────────────────────────────────── */

static void test_token_parser(void) {
    op_input_parsed_token_t t;

    /* Single character */
    t = op_input_parse_token("A");
    ASSERT_EQ(0x04, t.hid_code, "token 'A'");
    ASSERT_EQ(0, t.modifiers, "token 'A' no modifier");

    /* Special token */
    t = op_input_parse_token("<ENTER>");
    ASSERT_EQ(0x28, t.hid_code, "token <ENTER>");

    /* Function key */
    t = op_input_parse_token("<F5>");
    ASSERT_EQ(0x3E, t.hid_code, "token <F5>");

    /* Modifier opening — no key, no modifier on result */
    t = op_input_parse_token("<CTRL>");
    ASSERT_EQ(-1, t.hid_code, "modifier has no key");
    ASSERT_EQ(0, t.modifiers, "modifier result has no modifier flag");

    /* Delay token — ignored */
    t = op_input_parse_token("<DELAY1S>");
    ASSERT_EQ(-1, t.hid_code, "delay ignored");

    /* Unknown */
    t = op_input_parse_token("???");
    ASSERT_EQ(-1, t.hid_code, "unknown token");
}

/* ── Macro parser ──────────────────────────────────────────────────────── */

static void test_macro_parser(void) {
    op_input_parsed_token_t tokens[32];
    int n;

    /* Ctrl+C — modifier tags don't emit tokens, only the key */
    n = op_input_macro_parse("<CTRL>C</CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "Ctrl+C token count");
    ASSERT_EQ(0x06, tokens[0].hid_code, "C key code");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl modifier");

    /* Cmd+V */
    n = op_input_macro_parse("<CMD>V</CMD>", tokens, 32);
    ASSERT_EQ(1, n, "Cmd+V token count");
    ASSERT_EQ(0x19, tokens[0].hid_code, "V key code");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[0].modifiers, "GUI modifier");

    /* Ctrl+Alt+Del — only <DEL> emits (bare "DEL" is not a special token) */
    n = op_input_macro_parse("<CTRL><ALT><DEL></ALT></CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "Ctrl+Alt+Del token count");
    ASSERT_EQ(0x4C, tokens[0].hid_code, "Del key code");
    ASSERT_EQ(OP_INPUT_MOD_CTRL | OP_INPUT_MOD_ALT, tokens[0].modifiers, "Ctrl|Alt modifiers");

    /* Plain text */
    n = op_input_macro_parse("abc", tokens, 32);
    ASSERT_EQ(3, n, "'abc' token count");
    ASSERT_EQ(0x04, tokens[0].hid_code, "a");
    ASSERT_EQ(0x05, tokens[1].hid_code, "b");
    ASSERT_EQ(0x06, tokens[2].hid_code, "c");

    /* Text with shift symbols — letters are case-insensitive in macros */
    n = op_input_macro_parse("Hi!", tokens, 32);
    ASSERT_EQ(3, n, "'Hi!' token count");
    ASSERT_EQ(0, tokens[0].modifiers & OP_INPUT_MOD_SHIFT, "'H' (lowercased) no shift");
    ASSERT_NEQ(0, tokens[2].modifiers & OP_INPUT_MOD_SHIFT, "'!' has shift");

    /* Mixed macro with special keys */
    n = op_input_macro_parse("<F1><ENTER>", tokens, 32);
    ASSERT_EQ(2, n, "F1+Enter count");
    ASSERT_EQ(0x3A, tokens[0].hid_code, "F1");
    ASSERT_EQ(0x28, tokens[1].hid_code, "Enter");

    /* Delay tokens are skipped */
    n = op_input_macro_parse("<DELAY1S><CTRL>A</CTRL>", tokens, 32);
    ASSERT_EQ(1, n, "delay + Ctrl+A count");
    ASSERT_EQ(0x04, tokens[0].hid_code, "A after delay");
    ASSERT_EQ(OP_INPUT_MOD_CTRL, tokens[0].modifiers, "Ctrl modifier preserved");

    /* NULL / empty input */
    n = op_input_macro_parse(NULL, tokens, 32);
    ASSERT_EQ(0, n, "NULL input");
    n = op_input_macro_parse("", tokens, 32);
    ASSERT_EQ(0, n, "empty input");

    /* Max limit */
    n = op_input_macro_parse("abcdefghij", tokens, 3);
    ASSERT_EQ(3, n, "max limit respected");

    /* Modifier state persists across keys */
    n = op_input_macro_parse("<CMD>AB</CMD>", tokens, 32);
    ASSERT_EQ(2, n, "Cmd+AB count");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[0].modifiers, "A has GUI");
    ASSERT_EQ(OP_INPUT_MOD_GUI, tokens[1].modifiers, "B has GUI");

    /* Cmd+Shift+Z (redo) — macro parser lowercases letters, so Z → 'z' = 0x1D */
    n = op_input_macro_parse("<CMD><SHIFT>Z</SHIFT></CMD>", tokens, 32);
    ASSERT_EQ(1, n, "Cmd+Shift+Z count");
    ASSERT_EQ(0x1D, tokens[0].hid_code, "Z key code (lowercase)");
    ASSERT_EQ(OP_INPUT_MOD_GUI | OP_INPUT_MOD_SHIFT, tokens[0].modifiers, "GUI|Shift modifiers");
}

/* ── Hex dump ──────────────────────────────────────────────────────────── */

static void test_hex_dump(void) {
    uint8_t data[] = { 0x57, 0xAB, 0x00, 0x02 };
    char buf[32];
    op_input_hex_dump(data, 4, buf);
    ASSERT_STR_EQ("57AB0002", buf, "hex dump");

    op_input_hex_dump(data, 0, buf);
    ASSERT_STR_EQ("", buf, "hex dump empty");
}

/* ── Main ──────────────────────────────────────────────────────────────── */

int main(void) {
    printf("Running input tests...\n\n");

    test_checksum();
    test_keyboard_packet();
    test_mouse_packet();
    test_mouse_abs_packet();
    test_press_release();
    test_dom_code_mapping();
    test_hid_codes();
    test_char_to_hid();
    test_token_parser();
    test_macro_parser();
    test_hex_dump();

    if (failures == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }

    fprintf(stderr, "\n%d test(s) FAILED.\n", failures);
    return 1;
}
