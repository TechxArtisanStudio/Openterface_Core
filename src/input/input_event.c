#include <string.h>

#include "openterface/input.h"
#include "input_internal.h"

/* ── DOM event.code -> HID usage code mapping ─────────────────────────── */

static const name_entry_t dom_event_map[] = {
    /* Letters: KeyA-KeyZ -> 0x04-0x1D */
    { "KeyA", 0x04 }, { "KeyB", 0x05 }, { "KeyC", 0x06 }, { "KeyD", 0x07 },
    { "KeyE", 0x08 }, { "KeyF", 0x09 }, { "KeyG", 0x0A }, { "KeyH", 0x0B },
    { "KeyI", 0x0C }, { "KeyJ", 0x0D }, { "KeyK", 0x0E }, { "KeyL", 0x0F },
    { "KeyM", 0x10 }, { "KeyN", 0x11 }, { "KeyO", 0x12 }, { "KeyP", 0x13 },
    { "KeyQ", 0x14 }, { "KeyR", 0x15 }, { "KeyS", 0x16 }, { "KeyT", 0x17 },
    { "KeyU", 0x18 }, { "KeyV", 0x19 }, { "KeyW", 0x1A }, { "KeyX", 0x1B },
    { "KeyY", 0x1C }, { "KeyZ", 0x1D },
    /* Digits: Digit0-Digit9 -> 0x27,0x1E-0x26 */
    { "Digit0", 0x27 }, { "Digit1", 0x1E }, { "Digit2", 0x1F }, { "Digit3", 0x20 },
    { "Digit4", 0x21 }, { "Digit5", 0x22 }, { "Digit6", 0x23 }, { "Digit7", 0x24 },
    { "Digit8", 0x25 }, { "Digit9", 0x26 },
    /* Punctuation */
    { "Minus", 0x2D }, { "Equal", 0x2E },
    { "BracketLeft", 0x2F }, { "BracketRight", 0x30 },
    { "Backslash", 0x31 }, { "IntlBackslash", 0x64 },
    { "Semicolon", 0x33 }, { "Quote", 0x34 }, { "Backquote", 0x35 },
    { "Comma", 0x36 }, { "Period", 0x37 }, { "Slash", 0x38 },
    /* Action keys */
    { "Enter", 0x28 }, { "Escape", 0x29 }, { "Backspace", 0x2A },
    { "Tab", 0x2B }, { "Space", 0x2C }, { "CapsLock", 0x39 },
    /* Navigation */
    { "Insert", 0x49 }, { "Home", 0x4A }, { "PageUp", 0x4B },
    { "Delete", 0x4C }, { "End", 0x4D }, { "PageDown", 0x4E },
    { "ArrowRight", 0x4F }, { "ArrowLeft", 0x50 },
    { "ArrowDown", 0x51 }, { "ArrowUp", 0x52 },
    /* Function keys */
    { "F1", 0x3A }, { "F2", 0x3B }, { "F3", 0x3C }, { "F4", 0x3D },
    { "F5", 0x3E }, { "F6", 0x3F }, { "F7", 0x40 }, { "F8", 0x41 },
    { "F9", 0x42 }, { "F10", 0x43 }, { "F11", 0x44 }, { "F12", 0x45 },
    { "PrintScreen", 0x46 }, { "ScrollLock", 0x47 }, { "Pause", 0x48 },
    /* Numpad */
    { "NumLock", 0x53 },
    { "NumpadDivide", 0x54 }, { "NumpadMultiply", 0x55 },
    { "NumpadSubtract", 0x56 }, { "NumpadAdd", 0x57 },
    { "NumpadEnter", 0x58 },
    { "Numpad0", 0x62 }, { "Numpad1", 0x59 }, { "Numpad2", 0x5A },
    { "Numpad3", 0x5B }, { "Numpad4", 0x5C }, { "Numpad5", 0x5D },
    { "Numpad6", 0x5E }, { "Numpad7", 0x5F }, { "Numpad8", 0x60 },
    { "Numpad9", 0x61 },
    { "NumpadDecimal", 0x63 }, { "NumpadEqual", 0x67 },
    /* Modifiers */
    { "ControlLeft",  0xE0 }, { "ControlRight", 0xE4 },
    { "ShiftLeft",    0xE1 }, { "ShiftRight",   0xE5 },
    { "AltLeft",      0xE2 }, { "AltRight",     0xE6 },
    { "MetaLeft",     0xE3 }, { "MetaRight",    0xE7 },
};

static const int dom_event_map_len =
    (int)(sizeof(dom_event_map) / sizeof(dom_event_map[0]));

int op_input_hid_code_from_dom_code(const char *dom_code) {
    if (!dom_code || !dom_code[0]) return -1;
    for (int i = 0; i < dom_event_map_len; i++) {
        if (strcmp(dom_event_map[i].name, dom_code) == 0) {
            return dom_event_map[i].code;
        }
    }
    return -1;
}
