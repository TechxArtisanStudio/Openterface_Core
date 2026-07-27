#ifndef OPENTERFACE_CAPABILITY_H
#define OPENTERFACE_CAPABILITY_H

#include <stdint.h>

#include "openterface/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t op_capability_flags_t;

typedef enum {
    OP_CAPABILITY_NONE = 0,
    OP_CAPABILITY_INPUT_KEYBOARD = 1ull << 0,
    OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE = 1ull << 1,
    OP_CAPABILITY_INPUT_MOUSE_RELATIVE = 1ull << 2,
    OP_CAPABILITY_INPUT_MEDIA = 1ull << 3,
    OP_CAPABILITY_INPUT_MACRO = 1ull << 4,
    OP_CAPABILITY_DEVICE_INFO = 1ull << 5,
    OP_CAPABILITY_DEVICE_LOCK_LEDS = 1ull << 6,
    OP_CAPABILITY_VIDEO_CAPTURE = 1ull << 7,
    OP_CAPABILITY_VIDEO_INPUT_STATUS = 1ull << 8,
    OP_CAPABILITY_USB_ROLE_SWITCH = 1ull << 9,
    OP_CAPABILITY_SERIAL_BAUD_SWITCH = 1ull << 10,
    OP_CAPABILITY_CHIP_REGISTER_ACCESS = 1ull << 11,
    OP_CAPABILITY_FIRMWARE_INFO = 1ull << 12,
    OP_CAPABILITY_FIRMWARE_UPDATE = 1ull << 13,
    OP_CAPABILITY_AUDIO_CAPTURE = 1ull << 14
} op_capability_id_t;

typedef enum {
    OP_CAPABILITY_STATE_UNSUPPORTED = 0,
    OP_CAPABILITY_STATE_AVAILABLE = 1,
    OP_CAPABILITY_STATE_PERMISSION_REQUIRED = 2,
    OP_CAPABILITY_STATE_DISCONNECTED = 3,
    OP_CAPABILITY_STATE_BUSY = 4,
    OP_CAPABILITY_STATE_DEGRADED = 5,
    OP_CAPABILITY_STATE_REQUIRES_LOCAL_SERVICE = 6,
    OP_CAPABILITY_STATE_REQUIRES_FIRMWARE = 7
} op_capability_state_t;

int op_capabilities_has(op_capability_flags_t capabilities, op_capability_id_t capability);
op_capability_flags_t op_capabilities_input_basic(void);
const char *op_capability_label(op_capability_id_t capability);
const char *op_capability_state_label(op_capability_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_CAPABILITY_H */
