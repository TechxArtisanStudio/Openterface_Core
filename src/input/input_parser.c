#include <stdlib.h>
#include <string.h>

#include "openterface/input.h"
#include "input_internal.h"

/* ── Internal parser state ────────────────────────────────────────────── */

typedef struct {
    op_input_parsed_token_t *out;
    int count;
    int max;
    int active_mods;
} macro_parse_state_t;

/* ── Modifier token parsing ───────────────────────────────────────────── */

static int parse_modifier_token(const char *tok, int is_closing) {
    if (strcmp(tok, is_closing ? "</CTRL>" : "<CTRL>") == 0) return OP_INPUT_MOD_CTRL;
    if (strcmp(tok, is_closing ? "</SHIFT>" : "<SHIFT>") == 0) return OP_INPUT_MOD_SHIFT;
    if (strcmp(tok, is_closing ? "</ALT>" : "<ALT>") == 0) return OP_INPUT_MOD_ALT;
    if (strcmp(tok, is_closing ? "</CMD>" : "<CMD>") == 0) return OP_INPUT_MOD_GUI;
    if (strcmp(tok, is_closing ? "</WIN>" : "<WIN>") == 0) return OP_INPUT_MOD_GUI;
    return 0;
}

static int parse_modifier_open(const char *tok) {
    return parse_modifier_token(tok, 0);
}

static int parse_modifier_close(const char *tok) {
    return parse_modifier_token(tok, 1);
}

static int is_delay_token(const char *tok) {
    return strncmp(tok, "<DELAY", 6) == 0;
}

/* ── Tag extraction & special token lookup ────────────────────────────── */

static int extract_tag_inner(const char *tok, char *out, size_t out_size) {
    const char *inner = tok + 1;
    size_t len = strlen(inner);
    size_t inner_len;

    if (tok[0] != '<' || len < 2 || inner[len - 1] != '>') return 0;

    inner_len = len - 1;
    if (inner_len >= out_size) return 0;

    memcpy(out, inner, inner_len);
    out[inner_len] = '\0';
    return 1;
}

static int lookup_named_special_token(const char *name) {
    if (strcmp(name, "ENTER") == 0)    return 0x28;
    if (strcmp(name, "ESC") == 0)      return 0x29;
    if (strcmp(name, "BACK") == 0)     return 0x2A;
    if (strcmp(name, "TAB") == 0)      return 0x2B;
    if (strcmp(name, "SPACE") == 0)    return 0x2C;
    if (strcmp(name, "LEFT") == 0)     return 0x50;
    if (strcmp(name, "RIGHT") == 0)    return 0x4F;
    if (strcmp(name, "UP") == 0)       return 0x52;
    if (strcmp(name, "DOWN") == 0)     return 0x51;
    if (strcmp(name, "HOME") == 0)     return 0x4A;
    if (strcmp(name, "END") == 0)      return 0x4D;
    if (strcmp(name, "DEL") == 0)      return 0x4C;
    if (strcmp(name, "DELETE") == 0)   return 0x4C;
    if (strcmp(name, "INS") == 0)      return 0x49;
    if (strcmp(name, "INSERT") == 0)   return 0x49;
    if (strcmp(name, "PGUP") == 0)     return 0x4B;
    if (strcmp(name, "PAGEUP") == 0)   return 0x4B;
    if (strcmp(name, "PGDN") == 0)     return 0x4E;
    if (strcmp(name, "PAGEDOWN") == 0) return 0x4E;
    return -1;
}

static int function_key_hid(const char *name) {
    if (name[0] == 'F' && name[1] >= '0' && name[1] <= '9') {
        int num = atoi(name + 1);
        if (num >= 1 && num <= 12) {
            return (uint8_t)(0x3A + num - 1);
        }
    }
    return -1;
}

static int special_token_hid(const char *tok) {
    char buf[16];
    int hid;

    if (!extract_tag_inner(tok, buf, sizeof(buf))) return -1;

    hid = lookup_named_special_token(buf);
    if (hid >= 0) return hid;

    return function_key_hid(buf);
}

/* ── Token emission ───────────────────────────────────────────────────── */

static void emit_parsed_token(macro_parse_state_t *state, int hid_code, int modifiers) {
    if (hid_code < 0 || state->count >= state->max) return;
    state->out[state->count].hid_code = hid_code;
    state->out[state->count].modifiers = modifiers;
    state->count++;
}

static void parse_single_token(op_input_parsed_token_t *result, const char *token) {
    if (parse_modifier_open(token)) return;
    if (parse_modifier_close(token)) return;
    if (is_delay_token(token)) return;

    if (token[0] == '<') {
        result->hid_code = special_token_hid(token);
        return;
    }

    if (token[1] == '\0') {
        result->hid_code = op_input_hid_code_from_char(token[0], NULL);
    }
}

static void extract_tag_token(const char **cursor, char *out, size_t out_size) {
    const char *start = *cursor;
    const char *end = start;
    size_t token_length;

    while (*end && *end != '>') end++;
    if (*end == '>') end++;

    token_length = (size_t)(end - start);
    if (token_length >= out_size) token_length = out_size - 1;

    memcpy(out, start, token_length);
    out[token_length] = '\0';
    *cursor = end;
}

static char normalized_macro_char(char ch) {
    if (ch >= 'A' && ch <= 'Z') return (char)(ch - 'A' + 'a');
    return ch;
}

static void parse_macro_char(macro_parse_state_t *state, char ch) {
    int needs_shift = 0;
    int modifiers = state->active_mods;
    int hid = op_input_hid_code_from_char(normalized_macro_char(ch), &needs_shift);
    if (needs_shift) modifiers |= OP_INPUT_MOD_SHIFT;
    emit_parsed_token(state, hid, modifiers);
}

static void parse_macro_tag(macro_parse_state_t *state, const char *token) {
    int modifier = parse_modifier_open(token);
    if (modifier) {
        state->active_mods |= modifier;
        return;
    }

    modifier = parse_modifier_close(token);
    if (modifier) {
        state->active_mods &= ~modifier;
        return;
    }

    if (is_delay_token(token)) return;

    emit_parsed_token(state, special_token_hid(token), state->active_mods);
}

/* ── UTF-8 / script tokenization helpers ──────────────────────────────── */

static int utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static int is_script_tag_char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static int try_parse_script_tag(const char *input, int start, op_input_script_token_span_t *span) {
    int cursor = start + 1;
    int tag_start;

    if (input[start] != '<') return 0;
    if (input[cursor] == '/') cursor++;

    tag_start = cursor;
    while (input[cursor] != '\0' && is_script_tag_char((unsigned char)input[cursor])) {
        cursor++;
    }

    if (cursor <= tag_start || input[cursor] != '>') return 0;

    span->start_utf8 = start;
    span->length_utf8 = cursor + 1 - start;
    return 1;
}

static void write_script_span(op_input_script_token_span_t *span, int start, int length) {
    span->start_utf8 = start;
    span->length_utf8 = length;
}

/* ── Public parsing API ───────────────────────────────────────────────── */

op_input_parsed_token_t op_input_parse_token(const char *token) {
    op_input_parsed_token_t result;
    result.hid_code = -1;
    result.modifiers = 0;

    if (!token || !token[0]) return result;

    parse_single_token(&result, token);
    return result;
}

int op_input_macro_parse(const char *input, op_input_parsed_token_t out[], int max) {
    macro_parse_state_t state = { out, 0, max, 0 };
    const char *cursor;

    if (!input || !out || max <= 0) return 0;

    cursor = input;
    while (*cursor && state.count < state.max) {
        if (*cursor == '<') {
            char token[32];
            extract_tag_token(&cursor, token, sizeof(token));
            parse_macro_tag(&state, token);
            continue;
        }

        parse_macro_char(&state, *cursor);
        cursor++;
    }

    return state.count;
}

int op_input_script_tokenize(const char *input, op_input_script_token_span_t out[], int max) {
    int count = 0;
    int index = 0;

    if (!input || !out || max <= 0) return 0;

    while (input[index] != '\0' && count < max) {
        op_input_script_token_span_t span;
        int start = index;

        if (try_parse_script_tag(input, index, &span)) {
            out[count] = span;
            count++;
            index += span.length_utf8;
            continue;
        }

        index += utf8_char_len((unsigned char)input[index]);
        write_script_span(&out[count], start, index - start);
        count++;
    }

    return count;
}
