#ifndef OPENTERFACE_INPUT_INTERNAL_H
#define OPENTERFACE_INPUT_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t hid;
    uint8_t shift;
} ascii_entry_t;

typedef struct {
    const char *name;
    uint8_t code;
} name_entry_t;

/* HID lookup tables (defined in input_hid.c) */
extern const ascii_entry_t hid_ascii_map[96];
extern const name_entry_t hid_name_map[];
extern const int hid_name_map_len;

/* ASCII index helper (shared by input_hid.c and input_parser.c) */
int hid_ascii_index_for_char(char c);

#endif /* OPENTERFACE_INPUT_INTERNAL_H */
