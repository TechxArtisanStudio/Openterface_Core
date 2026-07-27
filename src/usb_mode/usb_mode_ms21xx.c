#include "usb_mode_internal.h"

#include "openterface/profile.h"

#define OP_USB_MODE_FIRMWARE_LEGACY_THRESHOLD 24081309u
#define OP_USB_MODE_FIRMWARE_MULTIPLIER_MAJOR 1000000u
#define OP_USB_MODE_FIRMWARE_MULTIPLIER_MINOR 10000u
#define OP_USB_MODE_FIRMWARE_MULTIPLIER_PATCH 100u

typedef struct {
    uint8_t enabled_bit;
    uint8_t clear_mask;
} op_usb_mode_bit_info_t;

static op_video_chip_controller_t *op_usb_mode_ms21xx_controller(op_usb_mode_endpoint_t *endpoint) {
    return endpoint != NULL ? endpoint->chip_controller : NULL;
}

static int op_usb_mode_ms21xx_is_chip_supported(op_video_chip_kind_t kind) {
    switch (kind) {
        case OP_VIDEO_CHIP_MS2109:
        case OP_VIDEO_CHIP_MS2109S:
        case OP_VIDEO_CHIP_MS2130S:
            return 1;
        default:
            return 0;
    }
}

static op_status_t op_video_chip_controller_read_firmware_version_code(op_video_chip_controller_t *controller, uint32_t *out_version_code) {
    op_video_chip_register_set_t registers;
    uint8_t part0 = 0u;
    uint8_t part1 = 0u;
    uint8_t part2 = 0u;
    uint8_t part3 = 0u;
    op_status_t status;

    if (controller == NULL || out_version_code == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_chip_controller_get_register_set(controller, &registers);
    if (status != OP_STATUS_OK) {
        return status;
    }

    if (registers.firmware_version[0] == 0u || registers.firmware_version[1] == 0u
        || registers.firmware_version[2] == 0u || registers.firmware_version[3] == 0u) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    status = op_video_chip_controller_read_register(controller, registers.firmware_version[0], &part0);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_video_chip_controller_read_register(controller, registers.firmware_version[1], &part1);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_video_chip_controller_read_register(controller, registers.firmware_version[2], &part2);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_video_chip_controller_read_register(controller, registers.firmware_version[3], &part3);
    if (status != OP_STATUS_OK) {
        return status;
    }

    *out_version_code = ((uint32_t)part0 * OP_USB_MODE_FIRMWARE_MULTIPLIER_MAJOR)
        + ((uint32_t)part1 * OP_USB_MODE_FIRMWARE_MULTIPLIER_MINOR)
        + ((uint32_t)part2 * OP_USB_MODE_FIRMWARE_MULTIPLIER_PATCH)
        + (uint32_t)part3;

    return OP_STATUS_OK;
}

static op_status_t op_video_chip_controller_get_spdif_register(op_video_chip_controller_t *controller, uint16_t *out_address) {
    op_video_chip_register_set_t registers;
    op_status_t status;

    if (controller == NULL || out_address == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_chip_controller_get_register_set(controller, &registers);
    if (status != OP_STATUS_OK) {
        return status;
    }

    if (registers.spdifout == 0u) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    *out_address = registers.spdifout;
    return OP_STATUS_OK;
}

static op_status_t op_video_chip_controller_get_usb_mode_bits(op_video_chip_controller_t *controller, op_usb_mode_bit_info_t *out_bits) {
    uint32_t version_code = 0u;
    op_status_t status;

    if (controller == NULL || out_bits == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_chip_controller_read_firmware_version_code(controller, &version_code);
    if (status == OP_STATUS_OK && version_code < OP_USB_MODE_FIRMWARE_LEGACY_THRESHOLD) {
        out_bits->enabled_bit = 0x10u;
        out_bits->clear_mask = 0xEFu;
        return OP_STATUS_OK;
    }

    out_bits->enabled_bit = 0x01u;
    out_bits->clear_mask = 0xFEu;
    return status == OP_STATUS_INVALID_ARGUMENT ? status : OP_STATUS_OK;
}

static int op_usb_mode_ms21xx_matches(const op_usb_mode_endpoint_t *endpoint) {
    op_video_chip_controller_t *controller = op_usb_mode_ms21xx_controller((op_usb_mode_endpoint_t *)endpoint);
    const op_device_info_t *device;

    if (controller == NULL || controller->session == NULL) {
        return 0;
    }

    device = op_hid_device_session_device(controller->session);
    if (device != NULL && (device->protocol_flags & (uint32_t)OP_PROTOCOL_MS21XX_REGISTER) != 0u) {
        return 1;
    }

    if (controller->chip_kind != OP_VIDEO_CHIP_UNKNOWN) {
        return op_usb_mode_ms21xx_is_chip_supported(controller->chip_kind);
    }

    if (device != NULL) {
        return op_usb_mode_ms21xx_is_chip_supported(op_video_chip_detect_from_device(device));
    }

    return 0;
}

static op_status_t op_usb_mode_ms21xx_get_mode(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t *out_mode) {
    op_video_chip_controller_t *controller = op_usb_mode_ms21xx_controller(endpoint);
    op_hid_transaction_guard_t *guard = NULL;
    op_usb_mode_bit_info_t bits;
    uint16_t spdifout_address = 0u;
    uint8_t spdifout_value = 0u;
    op_status_t status;

    if (controller == NULL || out_mode == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!op_hid_device_session_is_open(controller->session)) {
        return OP_STATUS_UNINITIALIZED;
    }

    status = op_hid_transaction_guard_create(controller->session, NULL, &guard);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_hid_transaction_guard_enter(guard);
    if (status != OP_STATUS_OK) {
        op_hid_transaction_guard_destroy(guard);
        return status;
    }

    status = op_video_chip_controller_get_spdif_register(controller, &spdifout_address);
    if (status == OP_STATUS_OK) {
        status = op_video_chip_controller_get_usb_mode_bits(controller, &bits);
    }
    if (status == OP_STATUS_OK) {
        status = op_video_chip_controller_read_register(controller, spdifout_address, &spdifout_value);
    }

    op_hid_transaction_guard_leave(guard);
    op_hid_transaction_guard_destroy(guard);

    if (status != OP_STATUS_OK) {
        return status;
    }

    *out_mode = (spdifout_value & bits.enabled_bit) != 0u ? OP_USB_MODE_TARGET : OP_USB_MODE_HOST;
    return OP_STATUS_OK;
}

static op_status_t op_usb_mode_ms21xx_set_mode(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t mode) {
    op_video_chip_controller_t *controller = op_usb_mode_ms21xx_controller(endpoint);
    op_hid_transaction_guard_t *guard = NULL;
    op_usb_mode_bit_info_t bits;
    uint16_t spdifout_address = 0u;
    uint8_t spdifout_value = 0u;
    op_status_t status;

    if (controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (mode != OP_USB_MODE_HOST && mode != OP_USB_MODE_TARGET) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!op_hid_device_session_is_open(controller->session)) {
        return OP_STATUS_UNINITIALIZED;
    }

    status = op_hid_transaction_guard_create(controller->session, NULL, &guard);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_hid_transaction_guard_enter(guard);
    if (status != OP_STATUS_OK) {
        op_hid_transaction_guard_destroy(guard);
        return status;
    }

    status = op_video_chip_controller_get_spdif_register(controller, &spdifout_address);
    if (status == OP_STATUS_OK) {
        status = op_video_chip_controller_get_usb_mode_bits(controller, &bits);
    }
    if (status == OP_STATUS_OK) {
        status = op_video_chip_controller_read_register(controller, spdifout_address, &spdifout_value);
    }
    if (status == OP_STATUS_OK) {
        if (mode == OP_USB_MODE_TARGET) {
            spdifout_value = (uint8_t)(spdifout_value | bits.enabled_bit);
        } else {
            spdifout_value = (uint8_t)(spdifout_value & bits.clear_mask);
        }
        status = op_video_chip_controller_write_register(controller, spdifout_address, spdifout_value);
    }

    op_hid_transaction_guard_leave(guard);
    op_hid_transaction_guard_destroy(guard);

    return status;
}

const op_usb_mode_provider_t *op_usb_mode_ms21xx_provider(void) {
    static const op_usb_mode_provider_t provider = {
        op_usb_mode_ms21xx_matches,
        op_usb_mode_ms21xx_get_mode,
        op_usb_mode_ms21xx_set_mode
    };

    return &provider;
}