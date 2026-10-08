/**
 * @file serial_chip.c
 * @brief Serial/HID chip type identification and capability queries.
 *
 * Pure logic — no I/O, no platform dependencies.
 */

#include "openterface/serial_chip_types.h"

/* ── Detection ─────────────────────────────────────────────────────── */

op_serial_chip_type_t op_serial_chip_detect(uint16_t vid, uint16_t pid)
{
    uint32_t vid_pid = (uint32_t)((uint32_t)vid << 16) | pid;

    if (vid_pid == ((uint32_t)OP_SERIAL_CHIP_CH9329_VID << 16 | OP_SERIAL_CHIP_CH9329_PID)) {
        return OP_SERIAL_CHIP_CH9329;
    }
    if (vid_pid == ((uint32_t)OP_SERIAL_CHIP_CH32V208_VID << 16 | OP_SERIAL_CHIP_CH32V208_PID)) {
        return OP_SERIAL_CHIP_CH32V208;
    }
    return OP_SERIAL_CHIP_UNKNOWN;
}

/* ── Name ──────────────────────────────────────────────────────────── */

const char *op_serial_chip_name(op_serial_chip_type_t type)
{
    switch (type) {
        case OP_SERIAL_CHIP_CH9329:    return "CH9329";
        case OP_SERIAL_CHIP_CH32V208:  return "CH32V208";
        case OP_SERIAL_CHIP_UNKNOWN:
        default:                       return "Unknown";
    }
}

/* ── Baudrate support ──────────────────────────────────────────────── */

bool op_serial_chip_supports_baudrate(op_serial_chip_type_t type, uint32_t baudrate)
{
    switch (type) {
        case OP_SERIAL_CHIP_CH9329:
            return baudrate == OP_SERIAL_CHIP_CH9329_BAUDRATE_LOW ||
                   baudrate == OP_SERIAL_CHIP_CH9329_BAUDRATE_HIGH;
        case OP_SERIAL_CHIP_CH32V208:
            return baudrate == OP_SERIAL_CHIP_CH32V208_BAUDRATE;
        default:
            return false;
    }
}

uint32_t op_serial_chip_default_baudrate(op_serial_chip_type_t type)
{
    switch (type) {
        case OP_SERIAL_CHIP_CH9329:    return OP_SERIAL_CHIP_CH9329_BAUDRATE_LOW;
        case OP_SERIAL_CHIP_CH32V208:  return OP_SERIAL_CHIP_CH32V208_BAUDRATE;
        default:                       return OP_SERIAL_CHIP_CH9329_BAUDRATE_LOW;
    }
}

/* ── Capability queries ────────────────────────────────────────────── */

bool op_serial_chip_supports_command_config(op_serial_chip_type_t type)
{
    switch (type) {
        case OP_SERIAL_CHIP_CH9329:    return true;
        case OP_SERIAL_CHIP_CH32V208:  return false;
        default:                       return true;  /* assume yes for unknown */
    }
}

bool op_serial_chip_supports_usb_switch(op_serial_chip_type_t type)
{
    switch (type) {
        case OP_SERIAL_CHIP_CH9329:    return false;
        case OP_SERIAL_CHIP_CH32V208:  return true;
        default:                       return false;
    }
}

/* ── Alternate baudrate ────────────────────────────────────────────── */

uint32_t op_serial_chip_alternate_baudrate(op_serial_chip_type_t type, uint32_t current_baudrate)
{
    if (type == OP_SERIAL_CHIP_CH9329) {
        return (current_baudrate == OP_SERIAL_CHIP_CH9329_BAUDRATE_HIGH)
                   ? OP_SERIAL_CHIP_CH9329_BAUDRATE_LOW
                   : OP_SERIAL_CHIP_CH9329_BAUDRATE_HIGH;
    }
    /* CH32V208 has no alternate; return same */
    return current_baudrate;
}
