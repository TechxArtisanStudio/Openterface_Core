#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openterface/input.h"

static void print_hex(const char *label, const uint8_t *data, int len) {
    char buf[256];
    op_input_hex_dump(data, len, buf);
    printf("%-20s %s\n", label, buf);
}

static void demo_keyboard(void) {
    printf("\n=== Keyboard Packets ===\n\n");

    /* 'A' key */
    uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, (uint8_t[]){ 0x04 }, 1);
    print_hex("'A' press:", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

    /* Ctrl+C */
    uint8_t keys[6] = { 0x06, 0, 0, 0, 0, 0 };
    op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL, keys, 1);
    print_hex("Ctrl+C:", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

    /* Cmd+V (macOS paste) */
    op_input_build_keyboard(pkt, OP_INPUT_MOD_GUI, keys, 1);
    keys[0] = 0x19; /* V */
    print_hex("Cmd+V:", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

    /* Key release */
    uint8_t zeros[6] = { 0 };
    op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, zeros, 0);
    print_hex("Release all:", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

    /* Multi-key: Ctrl+Alt+Del */
    keys[0] = 0x4C; /* Delete */
    op_input_build_keyboard(pkt, OP_INPUT_MOD_CTRL | OP_INPUT_MOD_ALT, keys, 1);
    print_hex("Ctrl+Alt+Del:", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

    /* Press + release combined */
    uint8_t pr[2 * OP_INPUT_PKT_KEYBOARD_SIZE];
    int n = op_input_build_press_release(pr, OP_INPUT_MOD_NONE, 0x28); /* Enter */
    printf("\nPress+Release Enter (%d bytes):\n", n);
    print_hex("  press:",  pr, OP_INPUT_PKT_KEYBOARD_SIZE);
    print_hex("  release:", pr + OP_INPUT_PKT_KEYBOARD_SIZE, OP_INPUT_PKT_KEYBOARD_SIZE);
}

static void demo_mouse(void) {
    printf("\n=== Mouse Packets ===\n\n");

    uint8_t pkt[OP_INPUT_PKT_MOUSE_REL_SIZE];

    /* Left click press */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_LEFT, 0, 0, 0);
    print_hex("Left click:", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);

    /* Left click release */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 0);
    print_hex("Release:", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);

    /* Right click */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_RIGHT, 0, 0, 0);
    print_hex("Right click:", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);

    /* Move: +50 right, -30 up */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 50, -30, 0);
    print_hex("Move (+50,-30):", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);

    /* Scroll up 3 notches */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, 3);
    print_hex("Scroll up 3:", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);

    /* Scroll down 5 notches */
    op_input_build_mouse_rel(pkt, OP_INPUT_MS_BTN_NONE, 0, 0, -5);
    print_hex("Scroll down 5:", pkt, OP_INPUT_PKT_MOUSE_REL_SIZE);
}

static void demo_hid_lookup(void) {
    printf("\n=== HID Code Lookup ===\n\n");

    const char *keys[] = {
        "A", "Z", "Enter", "Escape", "Backspace", "Tab", "Space",
        "F1", "F12", "Left", "Right", "Up", "Down",
        "Ctrl", "Shift", "Alt", "Cmd",
        "Numpad0", "NumpadEnter", "NumLock",
        NULL
    };

    for (int i = 0; keys[i]; i++) {
        int code = op_input_hid_code_from_name(keys[i]);
        printf("  %-15s -> 0x%02X (%d)\n", keys[i], code, code);
    }

    printf("\n  Char lookup:\n");
    for (char c = 'A'; c <= 'Z'; c++) {
        int need_shift = 0;
        int code = op_input_hid_code_from_char(c, &need_shift);
        printf("  '%c' -> 0x%02X shift=%d\n", c, code, need_shift);
    }
}

static void demo_macro_parse(void) {
    printf("\n=== Macro Parsing ===\n\n");

    const char *macros[] = {
        "<CTRL>C</CTRL>",
        "<CMD>V</CMD>",
        "<CTRL><ALT>DEL</ALT></CTRL>",
        "<CMD><SHIFT>Z</SHIFT></CMD>",
        "Hello World!",
        "abc123",
        "<F1><ENTER>",
        "<DELAY1S><CTRL>A</CTRL>",
        NULL
    };

    for (int i = 0; macros[i]; i++) {
        printf("  Input: %s\n", macros[i]);

        op_input_parsed_token_t tokens[32];
        int n = op_input_macro_parse(macros[i], tokens, 32);

        for (int j = 0; j < n; j++) {
            const char *label = op_input_hid_code_label(tokens[j].hid_code);
            printf("    [%d] key=0x%02X (%-10s) mods=0x%02X",
                   j, tokens[j].hid_code, label, tokens[j].modifiers);
            if (tokens[j].modifiers & OP_INPUT_MOD_CTRL)  printf(" CTRL");
            if (tokens[j].modifiers & OP_INPUT_MOD_SHIFT) printf(" SHIFT");
            if (tokens[j].modifiers & OP_INPUT_MOD_ALT)   printf(" ALT");
            if (tokens[j].modifiers & OP_INPUT_MOD_GUI)   printf(" GUI");
            printf("\n");
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        /* Single argument mode: interpret as a key name or macro */
        if (strcmp(argv[1], "--test") == 0) {
            demo_keyboard();
            demo_mouse();
            demo_hid_lookup();
            demo_macro_parse();
            return 0;
        }

        /* Try as a key name */
        int code = op_input_hid_code_from_name(argv[1]);
        if (code >= 0) {
            uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];
            op_input_build_keyboard(pkt, OP_INPUT_MOD_NONE, (uint8_t[]){ (uint8_t)code }, 1);
            print_hex(argv[1], pkt, OP_INPUT_PKT_KEYBOARD_SIZE);
            return 0;
        }

        /* Try as a macro string */
        op_input_parsed_token_t tokens[32];
        int n = op_input_macro_parse(argv[1], tokens, 32);
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                uint8_t pkt[OP_INPUT_PKT_KEYBOARD_SIZE];
                uint8_t keys[6] = { (uint8_t)tokens[i].hid_code };
                op_input_build_keyboard(pkt, tokens[i].modifiers, keys, 1);
                char label[64];
                snprintf(label, sizeof(label), "macro[%d]", i);
                print_hex(label, pkt, OP_INPUT_PKT_KEYBOARD_SIZE);

                /* Release */
                uint8_t zeros[6] = { 0 };
                op_input_build_keyboard(pkt, 0x00, zeros, 0);
                print_hex("release", pkt, OP_INPUT_PKT_KEYBOARD_SIZE);
            }
            return 0;
        }

        fprintf(stderr, "Unknown key or macro: %s\n", argv[1]);
        return 1;
    }

    /* Interactive demo */
    demo_keyboard();
    demo_mouse();
    demo_hid_lookup();
    demo_macro_parse();

    return 0;
}
