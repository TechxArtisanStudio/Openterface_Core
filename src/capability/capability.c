#include "openterface/capability.h"

int op_capabilities_has(op_capability_flags_t capabilities, op_capability_id_t capability) {
    if (capability == OP_CAPABILITY_NONE) {
        return 0;
    }

    return (capabilities & (op_capability_flags_t)capability) != 0u;
}

op_capability_flags_t op_capabilities_input_basic(void) {
    return (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO;
}

const char *op_capability_label(op_capability_id_t capability) {
    switch (capability) {
        case OP_CAPABILITY_INPUT_KEYBOARD:
            return "input.keyboard";
        case OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE:
            return "input.mouse.absolute";
        case OP_CAPABILITY_INPUT_MOUSE_RELATIVE:
            return "input.mouse.relative";
        case OP_CAPABILITY_INPUT_MEDIA:
            return "input.media";
        case OP_CAPABILITY_INPUT_MACRO:
            return "input.macro";
        case OP_CAPABILITY_DEVICE_INFO:
            return "device.info";
        case OP_CAPABILITY_DEVICE_LOCK_LEDS:
            return "device.lock_leds";
        case OP_CAPABILITY_VIDEO_CAPTURE:
            return "video.capture";
        case OP_CAPABILITY_VIDEO_INPUT_STATUS:
            return "video.input_status";
        case OP_CAPABILITY_USB_ROLE_SWITCH:
            return "usb.role_switch";
        case OP_CAPABILITY_SERIAL_BAUD_SWITCH:
            return "serial.baud_switch";
        case OP_CAPABILITY_CHIP_REGISTER_ACCESS:
            return "chip.register_access";
        case OP_CAPABILITY_FIRMWARE_INFO:
            return "firmware.info";
        case OP_CAPABILITY_FIRMWARE_UPDATE:
            return "firmware.update";
        case OP_CAPABILITY_AUDIO_CAPTURE:
            return "audio.capture";
        default:
            return "unknown";
    }
}

const char *op_capability_state_label(op_capability_state_t state) {
    switch (state) {
        case OP_CAPABILITY_STATE_AVAILABLE:
            return "available";
        case OP_CAPABILITY_STATE_PERMISSION_REQUIRED:
            return "permission_required";
        case OP_CAPABILITY_STATE_DISCONNECTED:
            return "disconnected";
        case OP_CAPABILITY_STATE_BUSY:
            return "busy";
        case OP_CAPABILITY_STATE_DEGRADED:
            return "degraded";
        case OP_CAPABILITY_STATE_REQUIRES_LOCAL_SERVICE:
            return "requires_local_service";
        case OP_CAPABILITY_STATE_REQUIRES_FIRMWARE:
            return "requires_firmware";
        default:
            return "unsupported";
    }
}
