#include "video_chip_controller_internal.h"

#include <stdlib.h>
#include <string.h>

struct op_video_status_poller_t {
    op_video_chip_controller_t *controller;
};

static op_status_t op_video_status_read_u8(op_video_chip_controller_t *controller, uint16_t address, uint8_t *out_value) {
    if (address == 0u) {
        if (out_value != NULL) {
            *out_value = 0u;
        }
        return OP_STATUS_NOT_SUPPORTED;
    }

    return op_video_chip_controller_read_register(controller, address, out_value);
}

static op_status_t op_video_status_read_u16(op_video_chip_controller_t *controller, uint16_t high_address, uint16_t low_address, uint16_t *out_value) {
    uint8_t high = 0u;
    uint8_t low = 0u;
    op_status_t status;

    if (out_value == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_status_read_u8(controller, high_address, &high);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_video_status_read_u8(controller, low_address, &low);
    if (status != OP_STATUS_OK) {
        return status;
    }

    *out_value = (uint16_t)(((uint16_t)high << 8) | (uint16_t)low);
    return OP_STATUS_OK;
}

static op_status_t op_video_status_poller_poll_live(op_video_status_poller_t *poller, op_video_input_status_t *out_status) {
    op_video_chip_register_set_t registers;
    op_video_chip_kind_t chip_kind = OP_VIDEO_CHIP_UNKNOWN;
    uint8_t hdmi_status = 0u;
    uint16_t raw_value = 0u;
    op_status_t status;

    status = op_video_chip_controller_detect(poller->controller, &chip_kind);
    if (status != OP_STATUS_OK) {
        return status;
    }

    status = op_video_chip_controller_get_register_set(poller->controller, &registers);
    if (status != OP_STATUS_OK) {
        return status;
    }

    op_video_input_status_clear(out_status);

    status = op_video_status_read_u8(poller->controller, registers.hdmi_status, &hdmi_status);
    if (status != OP_STATUS_OK) {
        return status;
    }

    out_status->hdmi_connected = (hdmi_status & 0x01u) != 0u;
    if (!out_status->hdmi_connected) {
        (void)op_video_chip_controller_set_cached_input_status(poller->controller, out_status);
        return OP_STATUS_OK;
    }

    status = op_video_status_read_u16(poller->controller, registers.width_h, registers.width_l, &raw_value);
    if (status != OP_STATUS_OK) {
        return status;
    }
    out_status->width = raw_value;

    status = op_video_status_read_u16(poller->controller, registers.height_h, registers.height_l, &raw_value);
    if (status != OP_STATUS_OK) {
        return status;
    }
    out_status->height = raw_value;

    status = op_video_status_read_u16(poller->controller, registers.fps_h, registers.fps_l, &raw_value);
    if (status != OP_STATUS_OK) {
        return status;
    }
    out_status->fps_milli = (uint32_t)raw_value * 10u;

    status = op_video_status_read_u16(poller->controller, registers.pixel_clock_h, registers.pixel_clock_l, &raw_value);
    if (status != OP_STATUS_OK) {
        return status;
    }
    out_status->pixel_clock_khz = (uint32_t)raw_value * 10u;

    (void)chip_kind;
    status = op_video_chip_controller_set_cached_input_status(poller->controller, out_status);
    if (status != OP_STATUS_OK) {
        return status;
    }

    return op_video_chip_controller_get_cached_input_status(poller->controller, out_status);
}

void op_video_input_status_clear(op_video_input_status_t *status) {
    if (status == NULL) {
        return;
    }

    memset(status, 0, sizeof(*status));
}

void op_video_input_status_normalize(op_video_chip_kind_t chip_kind, op_video_input_status_t *status) {
    if (status == NULL || !status->hdmi_connected) {
        return;
    }

    if (chip_kind == OP_VIDEO_CHIP_MS2109) {
        if (status->pixel_clock_khz > 189000u) {
            if (status->width != 4096u) {
                status->width = (uint16_t)(status->width * 2u);
            }
            if (status->height != 2160u) {
                status->height = (uint16_t)(status->height * 2u);
            }
        }
    } else if (chip_kind == OP_VIDEO_CHIP_MS2130S) {
        if (status->width == 3840u && status->height == 1080u) {
            status->height = 2160u;
        }
    }
}

op_status_t op_video_chip_controller_get_cached_input_status(op_video_chip_controller_t *controller, op_video_input_status_t *out_status) {
    if (controller == NULL || out_status == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memcpy(out_status, &controller->cached_status, sizeof(*out_status));
    return controller->cached_status.hdmi_connected ? OP_STATUS_OK : OP_STATUS_NOT_SUPPORTED;
}

op_status_t op_video_chip_controller_set_cached_input_status(op_video_chip_controller_t *controller, const op_video_input_status_t *status) {
    if (controller == NULL || status == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memcpy(&controller->cached_status, status, sizeof(controller->cached_status));
    op_video_input_status_normalize(controller->chip_kind, &controller->cached_status);
    return OP_STATUS_OK;
}


op_status_t op_video_status_poller_create(op_video_chip_controller_t *controller, op_video_status_poller_t **out_poller) {
    op_video_status_poller_t *poller;

    if (controller == NULL || out_poller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    poller = (op_video_status_poller_t *)calloc(1, sizeof(*poller));
    if (poller == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    poller->controller = controller;
    *out_poller = poller;
    return OP_STATUS_OK;
}

void op_video_status_poller_destroy(op_video_status_poller_t *poller) {
    free(poller);
}

op_status_t op_video_status_poller_poll(op_video_status_poller_t *poller, op_video_input_status_t *out_status) {
    op_status_t status;

    if (poller == NULL || out_status == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (poller->controller->chip_kind == OP_VIDEO_CHIP_UNKNOWN) {
        op_video_chip_kind_t kind = OP_VIDEO_CHIP_UNKNOWN;
        (void)op_video_chip_controller_detect(poller->controller, &kind);
    }

    if (!op_hid_device_session_is_open(poller->controller->session)) {
        return op_video_chip_controller_get_cached_input_status(poller->controller, out_status);
    }

    status = op_video_status_poller_poll_live(poller, out_status);
    if (status == OP_STATUS_OK) {
        return OP_STATUS_OK;
    }

    return op_video_chip_controller_get_cached_input_status(poller->controller, out_status);
}
