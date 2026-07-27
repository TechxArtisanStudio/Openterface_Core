#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "openterface/core_native.h"

#define TEST_PROTOCOL_CH9329 (1u << 0)
#define TEST_PROTOCOL_CH32V208 (1u << 1)
#define TEST_PROTOCOL_MS21XX_REGISTER (1u << 2)
#define TEST_CH9329_HEADER_0 0x57u
#define TEST_CH9329_HEADER_1 0xABu
#define TEST_CH9329_ADDR_DEFAULT 0x00u
#define TEST_CH9329_CMD_USB_SWITCH 0x17u
#define TEST_CH9329_RESP_USB_SWITCH (TEST_CH9329_CMD_USB_SWITCH | 0x80u)
#define TEST_CH9329_USB_SWITCH_HOST 0x00u
#define TEST_CH9329_USB_SWITCH_TARGET 0x01u
#define TEST_CH9329_USB_SWITCH_QUERY 0x03u
#define TEST_CH9329_PKT_USB_SWITCH_SIZE 11u
#define TEST_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE 7u

static int failures = 0;

#define ASSERT_TRUE(expr, msg) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL: %s\n", msg); \
        failures++; \
    } \
} while (0)

#define ASSERT_EQ_INT(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        fprintf(stderr, "FAIL: %s: expected %d, got %d\n", msg, (int)(expected), (int)(actual)); \
        failures++; \
    } \
} while (0)

typedef struct {
    int opened;
    op_usb_mode_t usb_mode;
    uint8_t last_request_type;
    uint8_t pending_response[TEST_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE];
    size_t pending_length;
} test_serial_transport_context_t;

static op_status_t test_serial_open(void *context) {
    test_serial_transport_context_t *serial = (test_serial_transport_context_t *)context;
    if (serial == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    serial->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t test_serial_close(void *context) {
    test_serial_transport_context_t *serial = (test_serial_transport_context_t *)context;
    if (serial == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    serial->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t test_serial_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    test_serial_transport_context_t *serial = (test_serial_transport_context_t *)context;

    if (serial == NULL || buffer == NULL || capacity < serial->pending_length) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!serial->opened || serial->pending_length == 0u) {
        return OP_STATUS_UNINITIALIZED;
    }

    memcpy(buffer, serial->pending_response, serial->pending_length);
    if (out_length != NULL) {
        *out_length = serial->pending_length;
    }
    serial->pending_length = 0u;
    return OP_STATUS_OK;
}

static op_status_t test_serial_write(void *context, const uint8_t *buffer, size_t length) {
    test_serial_transport_context_t *serial = (test_serial_transport_context_t *)context;
    uint8_t mode_status;

    if (serial == NULL || buffer == NULL || length != TEST_CH9329_PKT_USB_SWITCH_SIZE) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!serial->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    serial->last_request_type = buffer[9];
    if (serial->last_request_type == TEST_CH9329_USB_SWITCH_TARGET) {
        serial->usb_mode = OP_USB_MODE_TARGET;
    } else if (serial->last_request_type == TEST_CH9329_USB_SWITCH_HOST) {
        serial->usb_mode = OP_USB_MODE_HOST;
    }

    mode_status = serial->usb_mode == OP_USB_MODE_TARGET ? TEST_CH9329_USB_SWITCH_TARGET : TEST_CH9329_USB_SWITCH_HOST;
    serial->pending_response[0] = TEST_CH9329_HEADER_0;
    serial->pending_response[1] = TEST_CH9329_HEADER_1;
    serial->pending_response[2] = TEST_CH9329_ADDR_DEFAULT;
    serial->pending_response[3] = TEST_CH9329_RESP_USB_SWITCH;
    serial->pending_response[4] = 0x01u;
    serial->pending_response[5] = mode_status;
    serial->pending_response[6] = (uint8_t)((serial->pending_response[0] + serial->pending_response[1] + serial->pending_response[2] + serial->pending_response[3] + serial->pending_response[4] + serial->pending_response[5]) & 0xFFu);
    serial->pending_length = TEST_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE;
    return OP_STATUS_OK;
}

static op_status_t test_serial_control(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    (void)context;
    (void)request;
    (void)input;
    (void)input_length;
    (void)output;
    (void)output_capacity;
    (void)out_length;
    return OP_STATUS_NOT_SUPPORTED;
}

static void test_device_helpers(void) {
    op_device_info_t device;

    memset(&device, 0xAB, sizeof(device));
    op_device_info_init(&device);

    ASSERT_EQ_INT(0, device.vendor_id, "device init vendor_id");
    ASSERT_EQ_INT(0, device.product_id, "device init product_id");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_HID), "device init interfaces");

    device.interface_flags = OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA;
    ASSERT_EQ_INT(1, op_device_info_has_interface(&device, OP_DEVICE_IF_HID), "has HID interface");
    ASSERT_EQ_INT(1, op_device_info_has_interface(&device, OP_DEVICE_IF_CAMERA), "has camera interface");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_AUDIO), "missing audio interface");
}

static void test_chip_detection(void) {
    op_device_info_t device;

    op_device_info_init(&device);
    device.vendor_id = 0x345F;
    device.product_id = 0x2132;
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2130S, op_video_chip_detect_from_device(&device), "detect MS2130S by VID/PID");

    op_device_info_init(&device);
    strcpy(device.hid_path, "\\\\?\\hid#vid_345f&pid_2109");
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2109S, op_video_chip_detect_from_device(&device), "detect MS2109S by HID path");

    op_device_info_init(&device);
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2109, op_video_chip_detect_from_device(&device), "detect chip from hint");
    ASSERT_TRUE(strcmp(op_video_chip_kind_label(OP_VIDEO_CHIP_MS2109S), "MS2109S") == 0, "chip label");
}

static void test_controller_detect(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_chip_kind_t chip_kind = OP_VIDEO_CHIP_UNKNOWN;
    op_video_chip_register_set_t registers;
    op_status_t status;

    op_device_info_init(&device);
    strcpy(device.hid_path, "test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109S;

    status = op_hid_device_session_create(&device, &session);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "create session");

    status = op_video_chip_controller_create(session, &controller);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "create controller");

    status = op_video_chip_controller_detect(controller, &chip_kind);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "controller detect status");
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2109S, chip_kind, "controller detect kind");

    status = op_video_chip_controller_get_register_set(controller, &registers);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "controller register set status");
    ASSERT_EQ_INT(0xFD9C, registers.hdmi_status, "MS2109S HDMI register");
    ASSERT_EQ_INT(0xC703, registers.width_h, "MS2109S width_h register");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_hid_stub_register_access(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_chip_register_set_t registers;
    op_status_t status;
    uint8_t value = 0u;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;

    status = op_hid_device_session_create(&device, &session);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "create stub session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open stub session");

    status = op_video_chip_controller_create(session, &controller);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "create stub controller");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_detect(controller, &(op_video_chip_kind_t){OP_VIDEO_CHIP_UNKNOWN}), "detect stub chip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_register_set(controller, &registers), "get stub registers");

    status = op_video_chip_controller_read_register(controller, registers.width_h, &value);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "read width_h over HID");
    ASSERT_EQ_INT(0x07, value, "stub width_h value");

    status = op_video_chip_controller_write_register(controller, registers.spdifout, 0x11u);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "write spdifout over HID");
    status = op_video_chip_controller_read_register(controller, registers.spdifout, &value);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "read spdifout over HID");
    ASSERT_EQ_INT(0x11, value, "stub spdifout reflects write");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_video_status_helpers(void) {
    op_video_input_status_t status;
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_status_poller_t *poller = NULL;
    op_status_t result;

    op_video_input_status_clear(&status);
    ASSERT_EQ_INT(0, status.hdmi_connected, "cleared HDMI state");

    status.hdmi_connected = 1;
    status.width = 1920;
    status.height = 1080;
    status.pixel_clock_khz = 297000;
    op_video_input_status_normalize(OP_VIDEO_CHIP_MS2109, &status);
    ASSERT_EQ_INT(3840, status.width, "MS2109 width normalize");
    ASSERT_EQ_INT(2160, status.height, "MS2109 height normalize");

    status.hdmi_connected = 1;
    status.width = 3840;
    status.height = 1080;
    status.pixel_clock_khz = 148500;
    op_video_input_status_normalize(OP_VIDEO_CHIP_MS2130S, &status);
    ASSERT_EQ_INT(2160, status.height, "MS2130S height normalize");

    op_device_info_init(&device);
    strcpy(device.hid_path, "test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create session for status helper");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create controller for status helper");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_detect(controller, &(op_video_chip_kind_t){OP_VIDEO_CHIP_UNKNOWN}), "detect for status helper");

    status.hdmi_connected = 1;
    status.width = 1920;
    status.height = 1080;
    status.fps_milli = 60000;
    status.pixel_clock_khz = 297000;
    result = op_video_chip_controller_set_cached_input_status(controller, &status);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "set cached status");

    op_video_input_status_clear(&status);
    result = op_video_chip_controller_get_cached_input_status(controller, &status);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "get cached status");
    ASSERT_EQ_INT(3840, status.width, "cached normalized width");

    result = op_video_status_poller_create(controller, &poller);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "create poller");
    op_video_input_status_clear(&status);
    result = op_video_status_poller_poll(poller, &status);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "poll cached status");
    ASSERT_EQ_INT(2160, status.height, "poll normalized height");

    op_video_status_poller_destroy(poller);
    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_video_status_live_poll(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_status_poller_t *poller = NULL;
    op_video_input_status_t status;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;

    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create live poll session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open live poll session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create live poll controller");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_status_poller_create(controller, &poller), "create live poll poller");

    op_video_input_status_clear(&status);
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_status_poller_poll(poller, &status), "poll live HID status");
    ASSERT_EQ_INT(1, status.hdmi_connected, "live poll hdmi connected");
    ASSERT_EQ_INT(3840, status.width, "live poll normalized width");
    ASSERT_EQ_INT(2160, status.height, "live poll normalized height");
    ASSERT_EQ_INT(60000, (int)status.fps_milli, "live poll fps milli");
    ASSERT_EQ_INT(297000, (int)status.pixel_clock_khz, "live poll pixel clock");

    op_video_status_poller_destroy(poller);
    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_usb_mode_switch(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_chip_register_set_t registers;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;
    uint8_t value = 0u;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    device.protocol_flags = TEST_PROTOCOL_MS21XX_REGISTER;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;

    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create usb mode controller");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_detect(controller, &(op_video_chip_kind_t){OP_VIDEO_CHIP_UNKNOWN}), "detect usb mode chip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_register_set(controller, &registers), "get usb mode registers");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_usb_mode(controller, &mode), "initial usb mode read");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "initial usb mode is host");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_set_usb_mode(controller, OP_USB_MODE_TARGET), "set usb mode target");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_read_register(controller, registers.spdifout, &value), "read spdifout after target");
    ASSERT_EQ_INT(0x01, value, "modern firmware target bit set");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_usb_mode(controller, &mode), "get usb mode after target");
    ASSERT_EQ_INT(OP_USB_MODE_TARGET, mode, "usb mode target");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_set_usb_mode(controller, OP_USB_MODE_HOST), "set usb mode host");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_read_register(controller, registers.spdifout, &value), "read spdifout after host");
    ASSERT_EQ_INT(0x00, value, "modern firmware host bit cleared");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_usb_mode(controller, &mode), "get usb mode after host");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "usb mode host");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_usb_mode_switch_legacy_firmware(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_video_chip_register_set_t registers;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;
    uint8_t value = 0u;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    device.protocol_flags = TEST_PROTOCOL_MS21XX_REGISTER;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;

    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create legacy usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open legacy usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create legacy usb mode controller");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_detect(controller, &(op_video_chip_kind_t){OP_VIDEO_CHIP_UNKNOWN}), "detect legacy usb mode chip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_register_set(controller, &registers), "get legacy usb mode registers");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_write_register(controller, registers.firmware_version[0], 24u), "set legacy firmware part0");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_write_register(controller, registers.firmware_version[1], 8u), "set legacy firmware part1");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_write_register(controller, registers.firmware_version[2], 13u), "set legacy firmware part2");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_write_register(controller, registers.firmware_version[3], 8u), "set legacy firmware part3");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_set_usb_mode(controller, OP_USB_MODE_TARGET), "set legacy usb mode target");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_read_register(controller, registers.spdifout, &value), "read legacy spdifout after target");
    ASSERT_EQ_INT(0x10, value, "legacy firmware target bit set");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_usb_mode(controller, &mode), "get legacy usb mode after target");
    ASSERT_EQ_INT(OP_USB_MODE_TARGET, mode, "legacy usb mode target");

    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_set_usb_mode(controller, OP_USB_MODE_HOST), "set legacy usb mode host");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_read_register(controller, registers.spdifout, &value), "read legacy spdifout after host");
    ASSERT_EQ_INT(0x00, value, "legacy firmware target bit cleared");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_get_usb_mode(controller, &mode), "get legacy usb mode after host");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "legacy usb mode host");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_usb_mode_requires_capability(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    device.protocol_flags = TEST_PROTOCOL_MS21XX_REGISTER;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_VIDEO_CAPTURE;

    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create no-capability usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open no-capability usb mode session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create no-capability usb mode controller");

    ASSERT_EQ_INT(OP_STATUS_NOT_SUPPORTED, op_video_chip_controller_get_usb_mode(controller, &mode), "usb mode get requires capability");
    ASSERT_EQ_INT(OP_USB_MODE_UNKNOWN, mode, "usb mode remains unknown without capability");
    ASSERT_EQ_INT(OP_STATUS_NOT_SUPPORTED, op_video_chip_controller_set_usb_mode(controller, OP_USB_MODE_TARGET), "usb mode set requires capability");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_usb_mode_serial_provider(void) {
    op_device_info_t device;
    op_usb_mode_endpoint_t endpoint;
    op_transport_t transport;
    op_transport_vtable_t vtable = {
        test_serial_open,
        test_serial_close,
        test_serial_read,
        test_serial_write,
        test_serial_control
    };
    test_serial_transport_context_t serial = {0, OP_USB_MODE_HOST, 0u, {0u}, 0u};
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.protocol_flags = TEST_PROTOCOL_CH32V208;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;

    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &serial, &vtable);
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_open(&transport), "open serial transport");

    op_usb_mode_endpoint_init(&endpoint);
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_device(&endpoint, &device), "bind serial endpoint device");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_transport(&endpoint, &transport), "bind serial endpoint transport");

    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "serial endpoint get host mode");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "serial provider initial host mode");
    ASSERT_EQ_INT(TEST_CH9329_USB_SWITCH_QUERY, serial.last_request_type, "serial provider query request type");

    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_set(&endpoint, OP_USB_MODE_TARGET), "serial endpoint set target");
    ASSERT_EQ_INT(TEST_CH9329_USB_SWITCH_TARGET, serial.last_request_type, "serial provider target request type");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "serial endpoint get target mode");
    ASSERT_EQ_INT(OP_USB_MODE_TARGET, mode, "serial provider target mode");

    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_set(&endpoint, OP_USB_MODE_HOST), "serial endpoint set host");
    ASSERT_EQ_INT(TEST_CH9329_USB_SWITCH_HOST, serial.last_request_type, "serial provider host request type");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "serial endpoint get host mode after set");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "serial provider host mode after set");

    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_close(&transport), "close serial transport");
}

static void test_usb_mode_serial_requires_transport(void) {
    op_device_info_t device;
    op_usb_mode_endpoint_t endpoint;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.protocol_flags = TEST_PROTOCOL_CH32V208;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;

    op_usb_mode_endpoint_init(&endpoint);
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_device(&endpoint, &device), "bind no-transport endpoint device");

    ASSERT_EQ_INT(OP_STATUS_UNINITIALIZED, op_usb_mode_get(&endpoint, &mode), "serial endpoint get requires transport");
    ASSERT_EQ_INT(OP_STATUS_UNINITIALIZED, op_usb_mode_set(&endpoint, OP_USB_MODE_TARGET), "serial endpoint set requires transport");
}

static void test_usb_mode_endpoint_bind_controller(void) {
    op_device_info_t device;
    op_hid_device_session_t *session = NULL;
    op_video_chip_controller_t *controller = NULL;
    op_usb_mode_endpoint_t endpoint;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;

    op_device_info_init(&device);
    strcpy(device.hid_path, "stub:test-hid-path");
    device.chip_hint = OP_VIDEO_CHIP_MS2109;
    device.protocol_flags = TEST_PROTOCOL_MS21XX_REGISTER;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;

    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_create(&device, &session), "create endpoint-bind session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_hid_device_session_open(session), "open endpoint-bind session");
    ASSERT_EQ_INT(OP_STATUS_OK, op_video_chip_controller_create(session, &controller), "create endpoint-bind controller");

    op_usb_mode_endpoint_init(&endpoint);
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_chip_controller(&endpoint, controller), "bind endpoint controller");

    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "endpoint get usb mode via bound controller");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "endpoint bound controller initial host mode");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_set(&endpoint, OP_USB_MODE_TARGET), "endpoint set usb mode via bound controller");
    ASSERT_EQ_INT(OP_USB_MODE_TARGET, endpoint.cached_mode, "endpoint cache updated after set");

    op_video_chip_controller_destroy(controller);
    op_hid_device_session_destroy(session);
}

static void test_native_core_metadata(void) {
    op_version_t version = op_core_native_version();
    const char *backend_name = op_core_native_hid_backend_name();

    ASSERT_EQ_INT(0, version.major, "version major");
    ASSERT_EQ_INT(1, version.minor, "version minor");
    ASSERT_EQ_INT(0, version.patch, "version patch");
    ASSERT_TRUE(backend_name != NULL && backend_name[0] != '\0', "backend name is available");
}

static void test_native_serial_transport_backend(void) {
    op_device_info_t device;
    op_transport_t transport;
    op_usb_mode_endpoint_t endpoint;
    op_usb_mode_t mode = OP_USB_MODE_UNKNOWN;

    op_device_info_init(&device);
    strcpy(device.serial_path, "stub:serial-usb-mode");
    device.protocol_flags = TEST_PROTOCOL_CH32V208;
    device.capabilities = (op_capability_flags_t)OP_CAPABILITY_USB_ROLE_SWITCH;
    device.default_baudrate = 115200u;

    ASSERT_TRUE(strcmp(op_core_native_serial_backend_name(&device), "stub") == 0, "stub serial backend selected by device path");
    ASSERT_EQ_INT(OP_STATUS_OK, op_native_transport_init_serial(&transport, &device), "init native serial transport");
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_open(&transport), "open native serial transport");

    op_usb_mode_endpoint_init(&endpoint);
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_device(&endpoint, &device), "bind native serial device");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_endpoint_bind_transport(&endpoint, &transport), "bind native serial transport");

    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "native serial backend get host mode");
    ASSERT_EQ_INT(OP_USB_MODE_HOST, mode, "native serial backend initial host mode");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_set(&endpoint, OP_USB_MODE_TARGET), "native serial backend set target");
    ASSERT_EQ_INT(OP_STATUS_OK, op_usb_mode_get(&endpoint, &mode), "native serial backend get target mode");
    ASSERT_EQ_INT(OP_USB_MODE_TARGET, mode, "native serial backend target mode");

    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_close(&transport), "close native serial transport");
    op_native_transport_release(&transport);
}

int main(void) {
    test_device_helpers();
    test_chip_detection();
    test_controller_detect();
    test_hid_stub_register_access();
    test_video_status_helpers();
    test_video_status_live_poll();
    test_usb_mode_switch();
    test_usb_mode_switch_legacy_firmware();
    test_usb_mode_requires_capability();
    test_usb_mode_serial_provider();
    test_usb_mode_serial_requires_transport();
    test_usb_mode_endpoint_bind_controller();
    test_native_core_metadata();
    test_native_serial_transport_backend();

    if (failures != 0) {
        fprintf(stderr, "native_core_test: %d failure(s)\n", failures);
        return 1;
    }

    printf("native_core_test: all tests passed\n");
    return 0;
}
