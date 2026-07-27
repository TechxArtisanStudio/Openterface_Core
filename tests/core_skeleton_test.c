#include <stdio.h>
#include <string.h>

#include "openterface/core.h"

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

static op_status_t dummy_open(void *context) {
    int *opened = (int *)context;
    *opened = 1;
    return OP_STATUS_OK;
}

static op_status_t dummy_close(void *context) {
    int *opened = (int *)context;
    *opened = 0;
    return OP_STATUS_OK;
}

static op_status_t dummy_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    (void)context;
    if (buffer == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    buffer[0] = 0x42u;
    if (out_length != NULL) {
        *out_length = 1u;
    }
    return OP_STATUS_OK;
}

static op_status_t dummy_write(void *context, const uint8_t *buffer, size_t length) {
    (void)context;
    if (buffer == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    return OP_STATUS_OK;
}

static op_status_t dummy_control(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    (void)context;
    (void)request;
    (void)input;
    (void)input_length;
    if (output != NULL && output_capacity > 0u) {
        output[0] = 0u;
        if (out_length != NULL) {
            *out_length = 1u;
        }
    }
    return OP_STATUS_OK;
}

static void test_version(void) {
    op_version_t version = op_core_version();
    ASSERT_EQ_INT(0, version.major, "core version major");
    ASSERT_EQ_INT(1, version.minor, "core version minor");
    ASSERT_TRUE(strcmp(op_core_version_string(), "0.1.0") == 0, "core version string");
}

static void test_capabilities(void) {
    op_capability_flags_t capabilities = op_capabilities_input_basic();
    ASSERT_TRUE(op_capabilities_has(capabilities, OP_CAPABILITY_INPUT_KEYBOARD), "basic input has keyboard");
    ASSERT_TRUE(op_capabilities_has(capabilities, OP_CAPABILITY_INPUT_MACRO), "basic input has macro");
    ASSERT_TRUE(!op_capabilities_has(capabilities, OP_CAPABILITY_VIDEO_INPUT_STATUS), "basic input lacks video status");
    ASSERT_TRUE(strcmp(op_capability_label(OP_CAPABILITY_USB_ROLE_SWITCH), "usb.role_switch") == 0, "capability label");
    ASSERT_TRUE(strcmp(op_capability_state_label(OP_CAPABILITY_STATE_PERMISSION_REQUIRED), "permission_required") == 0, "capability state label");
}

static void test_profiles(void) {
    const op_device_profile_t *profile = op_profile_find_by_id("mini-kvm");
    op_device_info_t device;
    ASSERT_TRUE(profile != NULL, "find mini-kvm profile");
    ASSERT_EQ_INT(OP_DEVICE_FAMILY_MINI_KVM, profile->family, "mini-kvm family");
    ASSERT_EQ_INT(115200, profile->default_baudrate, "mini-kvm baudrate");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_INPUT_KEYBOARD), "mini-kvm keyboard capability");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_USB_ROLE_SWITCH), "mini-kvm usb switch capability");

    op_device_info_init(&device);
    ASSERT_EQ_INT(OP_STATUS_OK, op_profile_apply_to_device(profile, &device), "apply profile to device");
    ASSERT_TRUE(strcmp(device.profile_id, "mini-kvm") == 0, "device profile id");
    ASSERT_EQ_INT(0x1A86, device.vendor_id, "device vendor from profile");
    ASSERT_EQ_INT(115200, device.default_baudrate, "device baudrate from profile");
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_SERIAL), "device serial interface from profile");
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_USB_ROLE_SWITCH), "device usb role capability from profile");

    profile = op_profile_match(0x1A86, 0xFE0C, OP_DEVICE_IF_SERIAL);
    ASSERT_TRUE(profile != NULL, "match kvm-go serial profile");
    ASSERT_TRUE(strcmp(profile->profile_id, "kvm-go") == 0, "matched kvm-go id");

    profile = op_profile_match(0x1A86, 0x1234, OP_DEVICE_IF_SERIAL);
    ASSERT_TRUE(profile != NULL, "fallback keymod profile");
    ASSERT_TRUE(strcmp(profile->profile_id, "keymod") == 0, "fallback profile is keymod");
}

static void test_input_protocol_wrappers(void) {
    uint8_t packet[OP_CH9329_PKT_KEYBOARD_SIZE];
    uint8_t keys[2] = {0x04u, 0x05u};
    int len;

    ASSERT_EQ_INT(0x04, op_input_hid_code_from_name("A"), "op input HID from name");
    ASSERT_TRUE(strcmp(op_input_hid_code_label(0x28), "Enter") == 0, "op input HID label");

    len = op_ch9329_build_keyboard_packet(packet, OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_RSHIFT, keys, 2);
    ASSERT_EQ_INT(OP_CH9329_PKT_KEYBOARD_SIZE, len, "op ch9329 keyboard length");
    ASSERT_EQ_INT(OP_CH9329_HEADER_0, packet[0], "op ch9329 header 0");
    ASSERT_EQ_INT(OP_CH9329_CMD_KEYBOARD, packet[3], "op ch9329 command");
    ASSERT_EQ_INT(OP_INPUT_MOD_LCTRL | OP_INPUT_MOD_RSHIFT, packet[5], "op ch9329 modifier passthrough");
    ASSERT_EQ_INT(0x04, packet[7], "op ch9329 key slot 1");
    ASSERT_EQ_INT(0x05, packet[8], "op ch9329 key slot 2");
}

static void test_usb_switch_protocol(void) {
    uint8_t packet[OP_CH9329_PKT_USB_SWITCH_SIZE];
    uint8_t response[OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01u,
        OP_CH9329_USB_SWITCH_TARGET,
        0x00u
    };
    uint8_t status = 0xFFu;

    ASSERT_EQ_INT((int)OP_CH9329_PKT_USB_SWITCH_SIZE, op_ch9329_build_usb_switch_packet(packet, OP_CH9329_USB_SWITCH_QUERY), "usb switch packet length");
    ASSERT_EQ_INT(OP_CH9329_CMD_USB_SWITCH, packet[3], "usb switch packet command");
    ASSERT_EQ_INT(0x05, packet[4], "usb switch payload length");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_QUERY, packet[9], "usb switch query request value");

    response[6] = op_ch9329_checksum(response, (int)OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE);
    ASSERT_EQ_INT(OP_STATUS_OK, op_ch9329_parse_usb_switch_response(response, sizeof(response), &status), "usb switch response parse");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_TARGET, status, "usb switch response target status");
}

static void test_transport(void) {
    int opened = 0;
    uint8_t buffer[4];
    size_t length = 0u;
    op_transport_t transport;
    op_transport_vtable_t vtable = {
        dummy_open,
        dummy_close,
        dummy_read,
        dummy_write,
        dummy_control
    };

    op_transport_init(&transport, OP_TRANSPORT_KIND_STUB, &opened, &vtable);
    ASSERT_TRUE(strcmp(op_transport_kind_label(OP_TRANSPORT_KIND_STUB), "stub") == 0, "transport label");
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_open(&transport), "transport open");
    ASSERT_EQ_INT(1, opened, "transport opened context");
    ASSERT_EQ_INT(1, transport.is_open, "transport opened flag");
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_read(&transport, buffer, sizeof(buffer), &length), "transport read");
    ASSERT_EQ_INT(1, (int)length, "transport read length");
    ASSERT_EQ_INT(0x42, buffer[0], "transport read byte");
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_write(&transport, buffer, 1u), "transport write");
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_close(&transport), "transport close");
    ASSERT_EQ_INT(0, opened, "transport closed context");
}

int main(void) {
    test_version();
    test_capabilities();
    test_profiles();
    test_input_protocol_wrappers();
    test_usb_switch_protocol();
    test_transport();

    if (failures != 0) {
        fprintf(stderr, "core_skeleton_test: %d failure(s)\n", failures);
        return 1;
    }

    printf("core_skeleton_test: all tests passed\n");
    return 0;
}
