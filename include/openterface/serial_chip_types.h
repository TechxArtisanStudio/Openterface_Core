#ifndef OPENTERFACE_SERIAL_CHIP_TYPES_H
#define OPENTERFACE_SERIAL_CHIP_TYPES_H

/**
 * @file serial_chip_types.h
 * @brief Serial/HID chip type definitions for CH9329 and CH32V208 controllers.
 *
 * This header provides chip-type identification, VID/PID constants, and
 * lightweight query functions that any frontend (Qt, CLI, embedded) can use
 * without pulling in I/O dependencies.
 *
 * The actual serial-port operations (open, close, send commands) are the
 * responsibility of the frontend. This module only knows *what* each chip
 * supports, not *how* to talk to it over a real port.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Chip type enumeration ─────────────────────────────────────────── */

typedef enum {
    OP_SERIAL_CHIP_UNKNOWN = 0,
    OP_SERIAL_CHIP_CH9329,      /* VID:PID = 1A86:7523 */
    OP_SERIAL_CHIP_CH32V208,    /* VID:PID = 1A86:FE0C */
} op_serial_chip_type_t;

/* ── VID/PID constants ─────────────────────────────────────────────── */

#define OP_SERIAL_CHIP_CH9329_VID    0x1A86u
#define OP_SERIAL_CHIP_CH9329_PID    0x7523u

#define OP_SERIAL_CHIP_CH32V208_VID  0x1A86u
#define OP_SERIAL_CHIP_CH32V208_PID  0xFE0Cu

/* ── Baudrate constants ────────────────────────────────────────────── */

/** CH9329 supports both 9600 (default) and 115200 */
#define OP_SERIAL_CHIP_CH9329_BAUDRATE_LOW   9600u
#define OP_SERIAL_CHIP_CH9329_BAUDRATE_HIGH  115200u

/** CH32V208 only supports 115200 */
#define OP_SERIAL_CHIP_CH32V208_BAUDRATE     115200u

/* ── Query functions ───────────────────────────────────────────────── */

/**
 * Detect serial chip type from USB VID/PID.
 *
 * @param vid  USB Vendor ID
 * @param pid  USB Product ID
 * @return     The detected chip type, or OP_SERIAL_CHIP_UNKNOWN
 */
op_serial_chip_type_t op_serial_chip_detect(uint16_t vid, uint16_t pid);

/**
 * Get human-readable chip type name.
 *
 * @param type  Chip type
 * @return      Static string like "CH9329", "CH32V208", or "Unknown"
 */
const char *op_serial_chip_name(op_serial_chip_type_t type);

/**
 * Check whether a chip type supports a given baudrate.
 *
 * @param type      Chip type
 * @param baudrate  Baudrate to check
 * @return          true if the chip supports this baudrate
 */
bool op_serial_chip_supports_baudrate(op_serial_chip_type_t type, uint32_t baudrate);

/**
 * Get the default baudrate for a chip type.
 *
 * @param type  Chip type
 * @return      Default baudrate (9600 for CH9329, 115200 for CH32V208)
 */
uint32_t op_serial_chip_default_baudrate(op_serial_chip_type_t type);

/**
 * Check whether a chip type supports command-based configuration
 * (e.g. baudrate reconfiguration via serial commands).
 *
 * CH9329 supports it; CH32V208 does not.
 *
 * @param type  Chip type
 * @return      true if command-based configuration is supported
 */
bool op_serial_chip_supports_command_config(op_serial_chip_type_t type);

/**
 * Check whether a chip type supports USB switch via serial command.
 *
 * CH32V208 supports it; CH9329 does not.
 *
 * @param type  Chip type
 * @return      true if USB switch command is supported
 */
bool op_serial_chip_supports_usb_switch(op_serial_chip_type_t type);

/**
 * Get the alternate baudrate for fallback detection.
 *
 * For CH9329: toggles between 9600 and 115200.
 * For CH32V208: returns the same baudrate (no alternate).
 *
 * @param type            Chip type
 * @param current_baudrate Current baudrate
 * @return                The alternate baudrate to try
 */
uint32_t op_serial_chip_alternate_baudrate(op_serial_chip_type_t type, uint32_t current_baudrate);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_SERIAL_CHIP_TYPES_H */
