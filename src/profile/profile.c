#include "openterface/profile.h"

#include "openterface/chip.h"

#include <string.h>

#define OP_VENDOR_WCH 0x1A86u
#define OP_VENDOR_MACROSILICON_534D 0x534Du
#define OP_VENDOR_MACROSILICON_345F 0x345Fu
#define OP_PID_MINI_KVM_SERIAL 0x7523u
#define OP_PID_KVM_GO_SERIAL 0xFE0Cu
#define OP_PID_MS2109 0x2109u
#define OP_PID_MS2130S 0x2132u
#define OP_PROFILE_CHIP_UNKNOWN 0u
/* chip_hint values match op_video_chip_kind_t for compatibility with
   op_video_chip_detect_from_device */
#define OP_PROFILE_CHIP_MS2109  OP_VIDEO_CHIP_MS2109
#define OP_PROFILE_CHIP_MS2109S OP_VIDEO_CHIP_MS2109S
#define OP_PROFILE_CHIP_MS2130S OP_VIDEO_CHIP_MS2130S

static const op_device_profile_t OP_PROFILES[] = {
    {
        "mini-kvm",
        "Mini KVM",
        OP_DEVICE_FAMILY_MINI_KVM,
        OP_VENDOR_WCH,
        OP_PID_MINI_KVM_SERIAL,
        OP_DEVICE_IF_SERIAL | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_CH9329 | OP_PROTOCOL_MS21XX_REGISTER,
        115200u,
        OP_PROFILE_CHIP_MS2109,
        (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_LOCK_LEDS
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_SERIAL_BAUD_SWITCH
    },
    {
        "kvm-go",
        "KVM Go",
        OP_DEVICE_FAMILY_KVM_GO,
        OP_VENDOR_WCH,
        OP_PID_KVM_GO_SERIAL,
        OP_DEVICE_IF_SERIAL | OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_CH32V208 | OP_PROTOCOL_MS21XX_REGISTER,
        9600u,
        OP_PROFILE_CHIP_UNKNOWN,
        (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_LOCK_LEDS
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_SERIAL_BAUD_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    },
    {
        "kvm-go-gen3",
        "KVM Go Gen3 (USB 3.0)",
        OP_DEVICE_FAMILY_KVM_GO,
        OP_VENDOR_WCH,
        OP_PID_KVM_GO_SERIAL,
        OP_DEVICE_IF_SERIAL | OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_CH32V208 | OP_PROTOCOL_MS21XX_REGISTER,
        9600u,
        OP_PROFILE_CHIP_MS2130S,
        (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_LOCK_LEDS
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_SERIAL_BAUD_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    },
    {
        "kvm-go-v3",
        "KVM Go V3 (USB 3.0)",
        OP_DEVICE_FAMILY_KVM_GO,
        OP_VENDOR_WCH,
        OP_PID_KVM_GO_SERIAL,
        OP_DEVICE_IF_SERIAL | OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_CH32V208 | OP_PROTOCOL_MS21XX_REGISTER,
        9600u,
        OP_PROFILE_CHIP_MS2109S,
        (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_LOCK_LEDS
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_SERIAL_BAUD_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    },
    {
        "keymod",
        "KeyMod Input Device",
        OP_DEVICE_FAMILY_KEYMOD,
        OP_VENDOR_WCH,
        0u,
        OP_DEVICE_IF_SERIAL,
        OP_PROTOCOL_CH9329,
        115200u,
        OP_PROFILE_CHIP_UNKNOWN,
        (op_capability_flags_t)OP_CAPABILITY_INPUT_KEYBOARD
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MOUSE_RELATIVE
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MEDIA
        | (op_capability_flags_t)OP_CAPABILITY_INPUT_MACRO
        | (op_capability_flags_t)OP_CAPABILITY_DEVICE_INFO
    },
    {
        "ms2109-video",
        "MS2109 Video Companion",
        OP_DEVICE_FAMILY_MINI_KVM,
        OP_VENDOR_MACROSILICON_534D,
        OP_PID_MS2109,
        OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_MS21XX_REGISTER,
        0u,
        OP_PROFILE_CHIP_MS2109,
        (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    },
    {
        "ms2130s-video",
        "MS2130S Video Companion",
        OP_DEVICE_FAMILY_KVM_GO,
        OP_VENDOR_MACROSILICON_345F,
        OP_PID_MS2130S,
        OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_MS21XX_REGISTER,
        0u,
        OP_PROFILE_CHIP_MS2130S,
        (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    },
    {
        "ms2109s-video",
        "MS2109S Video Companion",
        OP_DEVICE_FAMILY_KVM_GO,
        OP_VENDOR_MACROSILICON_345F,
        OP_PID_MS2109,
        OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO,
        OP_PROTOCOL_MS21XX_REGISTER,
        0u,
        OP_PROFILE_CHIP_MS2109S,
        (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE
        | (op_capability_flags_t)OP_CAPABILITY_VIDEO_INPUT_STATUS
        | (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH
        | (op_capability_flags_t)OP_CAPABILITY_CHIP_REGISTER_ACCESS
        | (op_capability_flags_t)OP_CAPABILITY_FIRMWARE_INFO
    }
};

const op_device_profile_t *op_profile_registry(void) {
    return OP_PROFILES;
}

size_t op_profile_registry_count(void) {
    return sizeof(OP_PROFILES) / sizeof(OP_PROFILES[0]);
}

const op_device_profile_t *op_profile_find_by_id(const char *profile_id) {
    size_t index;

    if (profile_id == NULL || profile_id[0] == '\0') {
        return NULL;
    }

    for (index = 0u; index < op_profile_registry_count(); ++index) {
        if (strcmp(OP_PROFILES[index].profile_id, profile_id) == 0) {
            return &OP_PROFILES[index];
        }
    }

    return NULL;
}

const op_device_profile_t *op_profile_match(uint16_t vendor_id, uint16_t product_id, uint32_t interface_flags) {
    return op_profile_match_with_hint(vendor_id, product_id, interface_flags, OP_PROFILE_CHIP_UNKNOWN);
}

const op_device_profile_t *op_profile_match_with_hint(uint16_t vendor_id, uint16_t product_id, uint32_t interface_flags, uint32_t chip_hint) {
    size_t index;
    const op_device_profile_t *generic_keymod = NULL;
    const op_device_profile_t *best_match = NULL;

    for (index = 0u; index < op_profile_registry_count(); ++index) {
        const op_device_profile_t *profile = &OP_PROFILES[index];
        if (strcmp(profile->profile_id, "keymod") == 0) {
            generic_keymod = profile;
        }

        if (profile->vendor_id != vendor_id) {
            continue;
        }
        if (profile->product_id != 0u && profile->product_id != product_id) {
            continue;
        }
        if (interface_flags != 0u && (profile->interface_flags & interface_flags) == 0u) {
            continue;
        }
        if (profile->product_id == 0u && product_id != 0u) {
            continue;
        }

        /* If caller provided a chip hint, prefer profiles that match it */
        if (chip_hint != OP_PROFILE_CHIP_UNKNOWN) {
            if (profile->chip_hint == chip_hint) {
                return profile;
            }
            if (profile->chip_hint != OP_PROFILE_CHIP_UNKNOWN) {
                continue;
            }
        }

        /* Keep the first non-hinted match as fallback */
        if (best_match == NULL) {
            best_match = profile;
        }
    }

    if (best_match != NULL) {
        return best_match;
    }

    if (vendor_id == OP_VENDOR_WCH && generic_keymod != NULL) {
        return generic_keymod;
    }

    return NULL;
}

op_status_t op_profile_apply_to_device(const op_device_profile_t *profile, op_device_info_t *device) {
    if (profile == NULL || device == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    strncpy(device->profile_id, profile->profile_id, sizeof(device->profile_id) - 1u);
    device->profile_id[sizeof(device->profile_id) - 1u] = '\0';

    if (device->vendor_id == 0u) {
        device->vendor_id = profile->vendor_id;
    }
    if (device->product_id == 0u) {
        device->product_id = profile->product_id;
    }

    device->interface_flags |= profile->interface_flags;
    device->protocol_flags |= profile->protocol_flags;
    device->chip_hint = profile->chip_hint;
    device->default_baudrate = profile->default_baudrate;
    device->capabilities |= profile->capabilities;

    return OP_STATUS_OK;
}

int op_profile_has_capability(const op_device_profile_t *profile, op_capability_id_t capability) {
    if (profile == NULL) {
        return 0;
    }

    return op_capabilities_has(profile->capabilities, capability);
}

const char *op_device_family_label(op_device_family_t family) {
    switch (family) {
        case OP_DEVICE_FAMILY_MINI_KVM:
            return "mini-kvm";
        case OP_DEVICE_FAMILY_KVM_GO:
            return "kvm-go";
        case OP_DEVICE_FAMILY_KEYMOD:
            return "keymod";
        default:
            return "unknown";
    }
}
