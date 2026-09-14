#include "openterface/input.h"
#include "input_internal.h"

#include <string.h>

/* ── ASCII → HID mapping (printable 0x20–0x7E) ────────────────────────── */

const ascii_entry_t hid_ascii_map[96] = {
    /* 0x20 ' '  */ { 0x2C, 0 },
    /* 0x21 '!'  */ { 0x1E, 1 },
    /* 0x22 '"'  */ { 0x34, 1 },
    /* 0x23 '#'  */ { 0x20, 1 },
    /* 0x24 '$'  */ { 0x21, 1 },
    /* 0x25 '%'  */ { 0x22, 1 },
    /* 0x26 '&'  */ { 0x23, 1 },
    /* 0x27 '\''  */ { 0x34, 0 },
    /* 0x28 '('  */ { 0x26, 1 },
    /* 0x29 ')'  */ { 0x27, 1 },
    /* 0x2A '*'  */ { 0x25, 1 },
    /* 0x2B '+'  */ { 0x2E, 1 },
    /* 0x2C ','  */ { 0x36, 0 },
    /* 0x2D '-'  */ { 0x2D, 0 },
    /* 0x2E '.'  */ { 0x37, 0 },
    /* 0x2F '/'  */ { 0x38, 0 },
    /* 0x30 '0'  */ { 0x27, 0 },
    /* 0x31 '1'  */ { 0x1E, 0 },
    /* 0x32 '2'  */ { 0x1F, 0 },
    /* 0x33 '3'  */ { 0x20, 0 },
    /* 0x34 '4'  */ { 0x21, 0 },
    /* 0x35 '5'  */ { 0x22, 0 },
    /* 0x36 '6'  */ { 0x23, 0 },
    /* 0x37 '7'  */ { 0x24, 0 },
    /* 0x38 '8'  */ { 0x25, 0 },
    /* 0x39 '9'  */ { 0x26, 0 },
    /* 0x3A ':'  */ { 0x33, 1 },
    /* 0x3B ';'  */ { 0x33, 0 },
    /* 0x3C '<'  */ { 0x36, 1 },
    /* 0x3D '='  */ { 0x2E, 0 },
    /* 0x3E '>'  */ { 0x37, 1 },
    /* 0x3F '?'  */ { 0x38, 1 },
    /* 0x40 '@'  */ { 0x1F, 1 },
    /* 0x41 'A'  */ { 0x04, 1 },
    /* 0x42 'B'  */ { 0x05, 1 },
    /* 0x43 'C'  */ { 0x06, 1 },
    /* 0x44 'D'  */ { 0x07, 1 },
    /* 0x45 'E'  */ { 0x08, 1 },
    /* 0x46 'F'  */ { 0x09, 1 },
    /* 0x47 'G'  */ { 0x0A, 1 },
    /* 0x48 'H'  */ { 0x0B, 1 },
    /* 0x49 'I'  */ { 0x0C, 1 },
    /* 0x4A 'J'  */ { 0x0D, 1 },
    /* 0x4B 'K'  */ { 0x0E, 1 },
    /* 0x4C 'L'  */ { 0x0F, 1 },
    /* 0x4D 'M'  */ { 0x10, 1 },
    /* 0x4E 'N'  */ { 0x11, 1 },
    /* 0x4F 'O'  */ { 0x12, 1 },
    /* 0x50 'P'  */ { 0x13, 1 },
    /* 0x51 'Q'  */ { 0x14, 1 },
    /* 0x52 'R'  */ { 0x15, 1 },
    /* 0x53 'S'  */ { 0x16, 1 },
    /* 0x54 'T'  */ { 0x17, 1 },
    /* 0x55 'U'  */ { 0x18, 1 },
    /* 0x56 'V'  */ { 0x19, 1 },
    /* 0x57 'W'  */ { 0x1A, 1 },
    /* 0x58 'X'  */ { 0x1B, 1 },
    /* 0x59 'Y'  */ { 0x1C, 1 },
    /* 0x5A 'Z'  */ { 0x1D, 1 },
    /* 0x5B '['  */ { 0x2F, 0 },
    /* 0x5C '\\' */ { 0x64, 0 },
    /* 0x5D ']'  */ { 0x30, 0 },
    /* 0x5E '^'  */ { 0x23, 1 },
    /* 0x5F '_'  */ { 0x2D, 1 },
    /* 0x60 '`'  */ { 0x35, 0 },
    /* 0x61 'a'  */ { 0x04, 0 },
    /* 0x62 'b'  */ { 0x05, 0 },
    /* 0x63 'c'  */ { 0x06, 0 },
    /* 0x64 'd'  */ { 0x07, 0 },
    /* 0x65 'e'  */ { 0x08, 0 },
    /* 0x66 'f'  */ { 0x09, 0 },
    /* 0x67 'g'  */ { 0x0A, 0 },
    /* 0x68 'h'  */ { 0x0B, 0 },
    /* 0x69 'i'  */ { 0x0C, 0 },
    /* 0x6A 'j'  */ { 0x0D, 0 },
    /* 0x6B 'k'  */ { 0x0E, 0 },
    /* 0x6C 'l'  */ { 0x0F, 0 },
    /* 0x6D 'm'  */ { 0x10, 0 },
    /* 0x6E 'n'  */ { 0x11, 0 },
    /* 0x6F 'o'  */ { 0x12, 0 },
    /* 0x70 'p'  */ { 0x13, 0 },
    /* 0x71 'q'  */ { 0x14, 0 },
    /* 0x72 'r'  */ { 0x15, 0 },
    /* 0x73 's'  */ { 0x16, 0 },
    /* 0x74 't'  */ { 0x17, 0 },
    /* 0x75 'u'  */ { 0x18, 0 },
    /* 0x76 'v'  */ { 0x19, 0 },
    /* 0x77 'w'  */ { 0x1A, 0 },
    /* 0x78 'x'  */ { 0x1B, 0 },
    /* 0x79 'y'  */ { 0x1C, 0 },
    /* 0x7A 'z'  */ { 0x1D, 0 },
    /* 0x7B '{'  */ { 0x2F, 1 },
    /* 0x7C '|'  */ { 0x64, 1 },
    /* 0x7D '}'  */ { 0x30, 1 },
    /* 0x7E '~'  */ { 0x35, 1 },
};

/* ── Named key → HID mapping ──────────────────────────────────────────── */

const name_entry_t hid_name_map[] = {
    { "A", 0x04 }, { "B", 0x05 }, { "C", 0x06 }, { "D", 0x07 },
    { "E", 0x08 }, { "F", 0x09 }, { "G", 0x0A }, { "H", 0x0B },
    { "I", 0x0C }, { "J", 0x0D }, { "K", 0x0E }, { "L", 0x0F },
    { "M", 0x10 }, { "N", 0x11 }, { "O", 0x12 }, { "P", 0x13 },
    { "Q", 0x14 }, { "R", 0x15 }, { "S", 0x16 }, { "T", 0x17 },
    { "U", 0x18 }, { "V", 0x19 }, { "W", 0x1A }, { "X", 0x1B },
    { "Y", 0x1C }, { "Z", 0x1D },
    { "0", 0x27 }, { "1", 0x1E }, { "2", 0x1F }, { "3", 0x20 },
    { "4", 0x21 }, { "5", 0x22 }, { "6", 0x23 }, { "7", 0x24 },
    { "8", 0x25 }, { "9", 0x26 },
    { "Enter", 0x28 }, { "Escape", 0x29 }, { "Backspace", 0x2A },
    { "Tab", 0x2B }, { "Space", 0x2C }, { "Caps", 0x39 },
    { "Delete", 0x4C }, { "Insert", 0x49 }, { "Home", 0x4A },
    { "End", 0x4D }, { "PageUp", 0x4B }, { "PageDown", 0x4E },
    { "PgUp", 0x4B }, { "PgDn", 0x4E },
    { "Right", 0x4F }, { "Left", 0x50 }, { "Down", 0x51 }, { "Up", 0x52 },
    { "F1", 0x3A }, { "F2", 0x3B }, { "F3", 0x3C }, { "F4", 0x3D },
    { "F5", 0x3E }, { "F6", 0x3F }, { "F7", 0x40 }, { "F8", 0x41 },
    { "F9", 0x42 }, { "F10", 0x43 }, { "F11", 0x44 }, { "F12", 0x45 },
    { "Numpad0", 0x62 }, { "Numpad1", 0x59 }, { "Numpad2", 0x5A },
    { "Numpad3", 0x5B }, { "Numpad4", 0x5C }, { "Numpad5", 0x5D },
    { "Numpad6", 0x5E }, { "Numpad7", 0x5F }, { "Numpad8", 0x60 },
    { "Numpad9", 0x61 }, { "NumpadDot", 0x63 }, { "NumpadSlash", 0x54 },
    { "NumpadAsterisk", 0x55 }, { "NumpadMinus", 0x56 }, { "NumpadPlus", 0x57 },
    { "NumpadEnter", 0x58 }, { "NumpadEquals", 0x67 }, { "NumLock", 0x53 },
    { "Ctrl", 0xE0 }, { "Shift", 0xE1 }, { "Alt", 0xE2 }, { "Cmd", 0xE3 }, { "Win", 0xE3 },
    { "App", 0x65 }, { "ScrollLock", 0x47 }, { "PrtSc", 0x46 }, { "Pause", 0x48 },
};

const int hid_name_map_len = sizeof(hid_name_map) / sizeof(hid_name_map[0]);

/* ── Internal helpers ─────────────────────────────────────────────────── */

int hid_ascii_index_for_char(char c) {
    if (c < 0x20 || c > 0x7E) return -1;
    return c - 0x20;
}

static int hid_lookup_named_key_code(const char *key_name) {
    for (int i = 0; i < hid_name_map_len; i++) {
        if (strcmp(hid_name_map[i].name, key_name) == 0) {
            return hid_name_map[i].code;
        }
    }
    return -1;
}

static const char *hid_lookup_named_key_label(uint8_t hid_code) {
    for (int i = 0; i < hid_name_map_len; i++) {
        if (hid_name_map[i].code == hid_code) {
            return hid_name_map[i].name;
        }
    }
    return "Unknown";
}

/* ── Public HID lookup API ────────────────────────────────────────────── */

int op_input_hid_code_from_name(const char *key_name) {
    int idx;
    if (!key_name || !key_name[0]) return -1;
    idx = key_name[1] == '\0' ? hid_ascii_index_for_char(key_name[0]) : -1;
    if (idx >= 0) return hid_ascii_map[idx].hid;
    return hid_lookup_named_key_code(key_name);
}

int op_input_hid_code_from_char(char c, int *out_needs_shift) {
    int idx = hid_ascii_index_for_char(c);
    if (idx < 0) {
        if (out_needs_shift) *out_needs_shift = 0;
        return -1;
    }
    if (out_needs_shift) *out_needs_shift = hid_ascii_map[idx].shift;
    return hid_ascii_map[idx].hid;
}

const char *op_input_hid_code_label(int hid_code) {
    return hid_lookup_named_key_label((uint8_t)hid_code);
}
