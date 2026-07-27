#include "video_chip_controller_internal.h"

#include <stdlib.h>
#include <string.h>

#define OP_XDATA_READ_COMMAND 0xB5u
#define OP_XDATA_WRITE_COMMAND 0xB6u
#define OP_REPORT_SHORT_SIZE 9u
#define OP_REPORT_EXT_SIZE 11u
#define OP_REPORT_LARGE_SIZE 65u

typedef struct {
    uint8_t report_id;
    size_t report_size;
    size_t data_index;
} op_chip_read_attempt_t;

typedef struct {
    uint8_t report_id;
    size_t report_size;
} op_chip_write_attempt_t;

static op_status_t op_video_chip_controller_ensure_detected(op_video_chip_controller_t *controller) {
    op_video_chip_kind_t detected_kind;

    if (controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (controller->chip_kind != OP_VIDEO_CHIP_UNKNOWN) {
        return OP_STATUS_OK;
    }

    detected_kind = OP_VIDEO_CHIP_UNKNOWN;
    return op_video_chip_controller_detect(controller, &detected_kind);
}

static op_status_t op_video_chip_controller_transfer(
    op_hid_device_session_t *session,
    const uint8_t *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *out_response_length
) {
    op_status_t status;

    status = op_hid_device_session_send_feature_report(session, request, request_length);
    if (status != OP_STATUS_OK) {
        if (out_response_length != NULL) {
            *out_response_length = 0u;
        }
        return status;
    }

    if (response == NULL || response_capacity == 0u) {
        if (out_response_length != NULL) {
            *out_response_length = 0u;
        }
        return OP_STATUS_OK;
    }

    return op_hid_device_session_get_feature_report(session, response, response_capacity, out_response_length);
}

static op_status_t op_video_chip_controller_try_read_attempt(
    op_hid_device_session_t *session,
    const op_chip_read_attempt_t *attempt,
    uint16_t address,
    uint8_t *out_value
) {
    uint8_t request[OP_REPORT_LARGE_SIZE];
    uint8_t response[OP_REPORT_LARGE_SIZE];
    size_t response_length = 0u;
    op_status_t status;

    if (session == NULL || attempt == NULL || out_value == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memset(request, 0, sizeof(request));
    memset(response, 0, sizeof(response));

    request[0] = attempt->report_id;
    request[1] = OP_XDATA_READ_COMMAND;
    request[2] = (uint8_t)((address >> 8) & 0xFFu);
    request[3] = (uint8_t)(address & 0xFFu);
    response[0] = attempt->report_id;

    status = op_video_chip_controller_transfer(session, request, attempt->report_size, response, attempt->report_size, &response_length);
    if (status != OP_STATUS_OK) {
        return status;
    }

    if (response_length < attempt->data_index + 1u) {
        return OP_STATUS_IO_ERROR;
    }

    *out_value = response[attempt->data_index];
    return OP_STATUS_OK;
}

static op_status_t op_video_chip_controller_try_write_attempt(
    op_hid_device_session_t *session,
    const op_chip_write_attempt_t *attempt,
    uint16_t address,
    uint8_t value
) {
    uint8_t request[OP_REPORT_LARGE_SIZE];

    if (session == NULL || attempt == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memset(request, 0, sizeof(request));
    request[0] = attempt->report_id;
    request[1] = OP_XDATA_WRITE_COMMAND;
    request[2] = (uint8_t)((address >> 8) & 0xFFu);
    request[3] = (uint8_t)(address & 0xFFu);
    request[4] = value;

    return op_hid_device_session_send_feature_report(session, request, attempt->report_size);
}

static op_status_t op_video_chip_controller_read_register_with_attempts(
    op_hid_device_session_t *session,
    const op_chip_read_attempt_t *attempts,
    size_t attempt_count,
    uint16_t address,
    uint8_t *out_value
) {
    op_status_t last_status = OP_STATUS_NOT_SUPPORTED;
    size_t index;

    for (index = 0u; index < attempt_count; ++index) {
        op_status_t status = op_video_chip_controller_try_read_attempt(session, &attempts[index], address, out_value);
        if (status == OP_STATUS_OK) {
            return OP_STATUS_OK;
        }
        if (status == OP_STATUS_INVALID_ARGUMENT || status == OP_STATUS_UNINITIALIZED) {
            return status;
        }
        last_status = status;
    }

    return last_status;
}

static op_status_t op_video_chip_controller_write_register_with_attempts(
    op_hid_device_session_t *session,
    const op_chip_write_attempt_t *attempts,
    size_t attempt_count,
    uint16_t address,
    uint8_t value
) {
    op_status_t last_status = OP_STATUS_NOT_SUPPORTED;
    size_t index;

    for (index = 0u; index < attempt_count; ++index) {
        op_status_t status = op_video_chip_controller_try_write_attempt(session, &attempts[index], address, value);
        if (status == OP_STATUS_OK) {
            return OP_STATUS_OK;
        }
        if (status == OP_STATUS_INVALID_ARGUMENT || status == OP_STATUS_UNINITIALIZED) {
            return status;
        }
        last_status = status;
    }

    return last_status;
}

static op_video_chip_kind_t op_video_chip_detect_from_session(const op_hid_device_session_t *session) {
    const op_device_info_t *device = op_hid_device_session_device(session);
    return op_video_chip_detect_from_device(device);
}

op_status_t op_video_chip_controller_create(op_hid_device_session_t *session, op_video_chip_controller_t **out_controller) {
    op_video_chip_controller_t *controller;

    if (session == NULL || out_controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    controller = (op_video_chip_controller_t *)calloc(1, sizeof(*controller));
    if (controller == NULL) {
        return OP_STATUS_IO_ERROR;
    }

    controller->session = session;
    controller->transport = NULL;
    controller->chip_kind = OP_VIDEO_CHIP_UNKNOWN;
    controller->usb_mode = OP_USB_MODE_UNKNOWN;
    memset(&controller->cached_status, 0, sizeof(controller->cached_status));
    memset(&controller->register_set, 0, sizeof(controller->register_set));

    *out_controller = controller;
    return OP_STATUS_OK;
}

void op_video_chip_controller_destroy(op_video_chip_controller_t *controller) {
    free(controller);
}

op_status_t op_video_chip_controller_set_transport(op_video_chip_controller_t *controller, op_transport_t *transport) {
    if (controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    controller->transport = transport;
    return OP_STATUS_OK;
}

op_status_t op_video_chip_controller_detect(op_video_chip_controller_t *controller, op_video_chip_kind_t *out_kind) {
    if (controller == NULL || out_kind == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    controller->chip_kind = op_video_chip_detect_from_session(controller->session);
    if (controller->chip_kind != OP_VIDEO_CHIP_UNKNOWN) {
        (void)op_video_chip_get_register_set(controller->chip_kind, &controller->register_set);
    } else {
        memset(&controller->register_set, 0, sizeof(controller->register_set));
    }

    *out_kind = controller->chip_kind;
    return controller->chip_kind == OP_VIDEO_CHIP_UNKNOWN ? OP_STATUS_NOT_SUPPORTED : OP_STATUS_OK;
}

op_status_t op_video_chip_controller_get_register_set(op_video_chip_controller_t *controller, op_video_chip_register_set_t *out_registers) {
    if (controller == NULL || out_registers == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (controller->chip_kind == OP_VIDEO_CHIP_UNKNOWN) {
        controller->chip_kind = op_video_chip_detect_from_session(controller->session);
        if (controller->chip_kind == OP_VIDEO_CHIP_UNKNOWN) {
            memset(out_registers, 0, sizeof(*out_registers));
            return OP_STATUS_NOT_SUPPORTED;
        }
        (void)op_video_chip_get_register_set(controller->chip_kind, &controller->register_set);
    }

    memcpy(out_registers, &controller->register_set, sizeof(*out_registers));
    return OP_STATUS_OK;
}

op_status_t op_video_chip_controller_read_register(op_video_chip_controller_t *controller, uint16_t address, uint8_t *out_value) {
    static const op_chip_read_attempt_t ms2109_attempts[] = {
        {0x00u, OP_REPORT_SHORT_SIZE, 4u},
        {0x01u, OP_REPORT_SHORT_SIZE, 4u}
    };
    static const op_chip_read_attempt_t ms2109s_attempts[] = {
        {0x00u, OP_REPORT_EXT_SIZE, 4u},
        {0x01u, OP_REPORT_EXT_SIZE, 4u},
        {0x00u, OP_REPORT_LARGE_SIZE, 4u}
    };
    static const op_chip_read_attempt_t ms2130s_attempts[] = {
        {0x01u, OP_REPORT_EXT_SIZE, 4u},
        {0x00u, OP_REPORT_EXT_SIZE, 4u},
        {0x00u, OP_REPORT_LARGE_SIZE, 4u}
    };
    op_status_t status;

    if (controller == NULL || out_value == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_chip_controller_ensure_detected(controller);
    if (status != OP_STATUS_OK) {
        *out_value = 0u;
        return status;
    }

    if (!op_hid_device_session_is_open(controller->session)) {
        return OP_STATUS_UNINITIALIZED;
    }

    switch (controller->chip_kind) {
        case OP_VIDEO_CHIP_MS2109:
            return op_video_chip_controller_read_register_with_attempts(controller->session, ms2109_attempts, sizeof(ms2109_attempts) / sizeof(ms2109_attempts[0]), address, out_value);
        case OP_VIDEO_CHIP_MS2109S:
            return op_video_chip_controller_read_register_with_attempts(controller->session, ms2109s_attempts, sizeof(ms2109s_attempts) / sizeof(ms2109s_attempts[0]), address, out_value);
        case OP_VIDEO_CHIP_MS2130S:
            return op_video_chip_controller_read_register_with_attempts(controller->session, ms2130s_attempts, sizeof(ms2130s_attempts) / sizeof(ms2130s_attempts[0]), address, out_value);
        default:
            *out_value = 0u;
            return OP_STATUS_NOT_SUPPORTED;
    }
}

op_status_t op_video_chip_controller_write_register(op_video_chip_controller_t *controller, uint16_t address, uint8_t value) {
    static const op_chip_write_attempt_t ms2109_attempts[] = {
        {0x00u, OP_REPORT_SHORT_SIZE},
        {0x01u, OP_REPORT_SHORT_SIZE}
    };
    static const op_chip_write_attempt_t ms2109s_attempts[] = {
        {0x00u, OP_REPORT_SHORT_SIZE},
        {0x00u, OP_REPORT_EXT_SIZE}
    };
    static const op_chip_write_attempt_t ms2130s_attempts[] = {
        {0x01u, OP_REPORT_EXT_SIZE},
        {0x00u, OP_REPORT_EXT_SIZE},
        {0x00u, OP_REPORT_LARGE_SIZE}
    };
    op_status_t status;

    if (controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_video_chip_controller_ensure_detected(controller);
    if (status != OP_STATUS_OK) {
        return status;
    }

    if (!op_hid_device_session_is_open(controller->session)) {
        return OP_STATUS_UNINITIALIZED;
    }

    switch (controller->chip_kind) {
        case OP_VIDEO_CHIP_MS2109:
            return op_video_chip_controller_write_register_with_attempts(controller->session, ms2109_attempts, sizeof(ms2109_attempts) / sizeof(ms2109_attempts[0]), address, value);
        case OP_VIDEO_CHIP_MS2109S:
            return op_video_chip_controller_write_register_with_attempts(controller->session, ms2109s_attempts, sizeof(ms2109s_attempts) / sizeof(ms2109s_attempts[0]), address, value);
        case OP_VIDEO_CHIP_MS2130S:
            return op_video_chip_controller_write_register_with_attempts(controller->session, ms2130s_attempts, sizeof(ms2130s_attempts) / sizeof(ms2130s_attempts[0]), address, value);
        default:
            return OP_STATUS_NOT_SUPPORTED;
    }
}
