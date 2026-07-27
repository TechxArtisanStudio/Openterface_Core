#include "platform_backend.h"

#include "openterface/chip.h"
#include "openterface/usb_mode.h"

#include <stdlib.h>
#include <string.h>

#define OP_STUB_MAX_REPORT_SIZE 65u
#define OP_STUB_CMD_XDATA_READ 0xB5u
#define OP_STUB_CMD_XDATA_WRITE 0xB6u
#define OP_STUB_SERIAL_MAX_PACKET_SIZE 16u
#define OP_STUB_SERIAL_USB_SWITCH_CMD 0x17u
#define OP_STUB_SERIAL_USB_SWITCH_RESP 0x97u

typedef struct {
    op_device_info_t device;
    op_video_chip_kind_t chip_kind;
    uint8_t registers[65536];
    uint8_t pending_report[OP_STUB_MAX_REPORT_SIZE];
    size_t pending_length;
    int opened;
} op_platform_stub_context_t;

typedef struct {
    op_device_info_t device;
    uint8_t pending_response[OP_STUB_SERIAL_MAX_PACKET_SIZE];
    size_t pending_length;
    op_usb_mode_t usb_mode;
    int opened;
} op_platform_stub_serial_context_t;

static uint8_t op_platform_stub_serial_checksum(const uint8_t *buffer, size_t length_without_checksum) {
    size_t index;
    uint32_t sum = 0u;

    for (index = 0u; index < length_without_checksum; ++index) {
        sum += buffer[index];
    }

    return (uint8_t)(sum & 0xFFu);
}

static op_status_t op_platform_stub_serial_prepare_response(op_platform_stub_serial_context_t *stub, const uint8_t *buffer, size_t length) {
    uint8_t request_type;
    uint8_t mode_status;

    if (stub == NULL || buffer == NULL || length < 10u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (buffer[0] != 0x57u || buffer[1] != 0xABu || buffer[3] != OP_STUB_SERIAL_USB_SWITCH_CMD) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    request_type = buffer[9];
    if (request_type == 0x01u) {
        stub->usb_mode = OP_USB_MODE_TARGET;
    } else if (request_type == 0x00u) {
        stub->usb_mode = OP_USB_MODE_HOST;
    }

    mode_status = stub->usb_mode == OP_USB_MODE_TARGET ? 0x01u : 0x00u;
    stub->pending_response[0] = 0x57u;
    stub->pending_response[1] = 0xABu;
    stub->pending_response[2] = 0x00u;
    stub->pending_response[3] = OP_STUB_SERIAL_USB_SWITCH_RESP;
    stub->pending_response[4] = 0x01u;
    stub->pending_response[5] = mode_status;
    stub->pending_response[6] = op_platform_stub_serial_checksum(stub->pending_response, 6u);
    stub->pending_length = 7u;
    return OP_STATUS_OK;
}

static void op_platform_stub_write_u8(op_platform_stub_context_t *stub, uint16_t address, uint8_t value) {
    if (stub == NULL) {
        return;
    }

    stub->registers[address] = value;
}

static void op_platform_stub_write_u16(op_platform_stub_context_t *stub, uint16_t high_address, uint16_t low_address, uint16_t value) {
    op_platform_stub_write_u8(stub, high_address, (uint8_t)((value >> 8) & 0xFFu));
    op_platform_stub_write_u8(stub, low_address, (uint8_t)(value & 0xFFu));
}

static void op_platform_stub_seed_registers(op_platform_stub_context_t *stub) {
    op_video_chip_register_set_t registers;

    if (stub == NULL) {
        return;
    }

    memset(&registers, 0, sizeof(registers));
    if (!op_video_chip_get_register_set(stub->chip_kind, &registers)) {
        return;
    }

    if (registers.hdmi_status != 0u) {
        op_platform_stub_write_u8(stub, registers.hdmi_status, 0x01u);
    }
    if (registers.gpio0 != 0u) {
        op_platform_stub_write_u8(stub, registers.gpio0, 0x01u);
    }
    if (registers.spdifout != 0u) {
        op_platform_stub_write_u8(stub, registers.spdifout, 0x00u);
    }

    op_platform_stub_write_u16(stub, registers.width_h, registers.width_l, 1920u);
    op_platform_stub_write_u16(stub, registers.height_h, registers.height_l, 1080u);
    op_platform_stub_write_u16(stub, registers.fps_h, registers.fps_l, 6000u);
    op_platform_stub_write_u16(stub, registers.pixel_clock_h, registers.pixel_clock_l, 29700u);

    if (registers.firmware_version[0] != 0u) {
        op_platform_stub_write_u8(stub, registers.firmware_version[0], 24u);
        op_platform_stub_write_u8(stub, registers.firmware_version[1], 8u);
        op_platform_stub_write_u8(stub, registers.firmware_version[2], 13u);
        op_platform_stub_write_u8(stub, registers.firmware_version[3], 9u);
    }
}

static op_status_t op_platform_stub_prepare_response(op_platform_stub_context_t *stub, const uint8_t *report, size_t length) {
    uint16_t address;
    size_t index;

    if (stub == NULL || report == NULL || length < 4u || length > OP_STUB_MAX_REPORT_SIZE) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    address = (uint16_t)(((uint16_t)report[2] << 8) | (uint16_t)report[3]);
    memset(stub->pending_report, 0, sizeof(stub->pending_report));
    stub->pending_length = length;
    stub->pending_report[0] = report[0];
    stub->pending_report[1] = report[1];
    stub->pending_report[2] = report[2];
    stub->pending_report[3] = report[3];

    switch (report[1]) {
        case OP_STUB_CMD_XDATA_READ:
            stub->pending_report[4] = stub->registers[address];
            return OP_STATUS_OK;
        case OP_STUB_CMD_XDATA_WRITE:
            for (index = 4u; index < length && index < 8u; ++index) {
                stub->registers[(uint16_t)(address + (uint16_t)(index - 4u))] = report[index];
                stub->pending_report[index] = report[index];
            }
            return OP_STATUS_OK;
        default:
            stub->pending_length = 0u;
            return OP_STATUS_NOT_SUPPORTED;
    }
}

static void *op_platform_stub_create_context(const op_device_info_t *device) {
    op_platform_stub_context_t *context = (op_platform_stub_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
        context->chip_kind = op_video_chip_detect_from_device(device);
    }

    op_platform_stub_seed_registers(context);

    return context;
}

static void op_platform_stub_destroy_context(void *context) {
    free(context);
}

static op_status_t op_platform_stub_open(void *context) {
    op_platform_stub_context_t *stub = (op_platform_stub_context_t *)context;
    if (stub == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (stub->device.hid_path[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    stub->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_stub_close(void *context) {
    op_platform_stub_context_t *stub = (op_platform_stub_context_t *)context;
    if (stub == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    stub->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_stub_send_feature_report(void *context, const uint8_t *report, size_t length) {
    op_platform_stub_context_t *stub = (op_platform_stub_context_t *)context;
    if (stub == NULL || report == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!stub->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    return op_platform_stub_prepare_response(stub, report, length);
}

static op_status_t op_platform_stub_get_feature_report(void *context, uint8_t *report, size_t capacity, size_t *out_length) {
    op_platform_stub_context_t *stub = (op_platform_stub_context_t *)context;
    if (stub == NULL || report == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!stub->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (stub->pending_length == 0u) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return OP_STATUS_NOT_SUPPORTED;
    }

    if (capacity < stub->pending_length) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memcpy(report, stub->pending_report, stub->pending_length);

    if (out_length != NULL) {
        *out_length = stub->pending_length;
    }

    return OP_STATUS_OK;
}

static const op_platform_hid_backend_t OP_PLATFORM_STUB_BACKEND = {
    op_platform_stub_create_context,
    op_platform_stub_destroy_context,
    op_platform_stub_open,
    op_platform_stub_close,
    op_platform_stub_send_feature_report,
    op_platform_stub_get_feature_report,
};

const op_platform_hid_backend_t *op_platform_get_stub_hid_backend(void) {
    return &OP_PLATFORM_STUB_BACKEND;
}

static void *op_platform_stub_serial_create_context(const op_device_info_t *device) {
    op_platform_stub_serial_context_t *context = (op_platform_stub_serial_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
    }
    context->usb_mode = OP_USB_MODE_HOST;
    return context;
}

static void op_platform_stub_serial_destroy_context(void *context) {
    free(context);
}

static op_status_t op_platform_stub_serial_open(void *context) {
    op_platform_stub_serial_context_t *stub = (op_platform_stub_serial_context_t *)context;
    if (stub == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (stub->device.serial_path[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    stub->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_stub_serial_close(void *context) {
    op_platform_stub_serial_context_t *stub = (op_platform_stub_serial_context_t *)context;
    if (stub == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    stub->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_stub_serial_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    op_platform_stub_serial_context_t *stub = (op_platform_stub_serial_context_t *)context;
    if (stub == NULL || buffer == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!stub->opened) {
        return OP_STATUS_UNINITIALIZED;
    }
    if (stub->pending_length == 0u) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return OP_STATUS_TIMEOUT;
    }
    if (capacity < stub->pending_length) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    memcpy(buffer, stub->pending_response, stub->pending_length);
    if (out_length != NULL) {
        *out_length = stub->pending_length;
    }
    stub->pending_length = 0u;
    return OP_STATUS_OK;
}

static op_status_t op_platform_stub_serial_write(void *context, const uint8_t *buffer, size_t length) {
    op_platform_stub_serial_context_t *stub = (op_platform_stub_serial_context_t *)context;
    if (stub == NULL || buffer == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!stub->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    return op_platform_stub_serial_prepare_response(stub, buffer, length);
}

static const op_platform_serial_backend_t OP_PLATFORM_STUB_SERIAL_BACKEND = {
    op_platform_stub_serial_create_context,
    op_platform_stub_serial_destroy_context,
    op_platform_stub_serial_open,
    op_platform_stub_serial_close,
    op_platform_stub_serial_read,
    op_platform_stub_serial_write,
};

const op_platform_serial_backend_t *op_platform_get_stub_serial_backend(void) {
    return &OP_PLATFORM_STUB_SERIAL_BACKEND;
}
