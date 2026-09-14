/*
 * device_test.c - Tests for the device info module
 *
 * Tests device info initialization, interface/capability queries,
 * and field management.
 */

#include "openterface/device.h"
#include "openterface/capability.h"
#include "test_helpers.h"

int test_failures = 0;

static void test_device_info_init(void) {
    op_device_info_t device;

    memset(&device, 0xFF, sizeof(device));
    op_device_info_init(&device);

    ASSERT_EQ_INT(0, device.vendor_id, "vendor_id is 0");
    ASSERT_EQ_INT(0, device.product_id, "product_id is 0");
    ASSERT_EQ_INT(0, (int)device.interface_flags, "interface_flags is 0");
    ASSERT_EQ_INT(0, (int)device.protocol_flags, "protocol_flags is 0");
    ASSERT_EQ_INT(0, (int)device.chip_hint, "chip_hint is 0");
    ASSERT_EQ_INT(0, (int)device.default_baudrate, "default_baudrate is 0");
    ASSERT_EQ_INT(0, (int)strlen(device.device_id), "device_id is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.profile_id), "profile_id is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.port_chain), "port_chain is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.hid_path), "hid_path is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.serial_path), "serial_path is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.camera_path), "camera_path is empty");
    ASSERT_EQ_INT(0, (int)strlen(device.audio_path), "audio_path is empty");
}

static void test_device_info_init_null(void) {
    /* Should not crash */
    op_device_info_init(NULL);
}

static void test_device_info_has_interface(void) {
    op_device_info_t device;
    op_device_info_init(&device);

    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_HID), "initial lacks HID");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_SERIAL), "initial lacks SERIAL");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_CAMERA), "initial lacks CAMERA");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_AUDIO), "initial lacks AUDIO");

    device.interface_flags = OP_DEVICE_IF_HID;
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_HID) != 0, "has HID");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_SERIAL), "lacks SERIAL");

    device.interface_flags = OP_DEVICE_IF_HID | OP_DEVICE_IF_SERIAL | OP_DEVICE_IF_CAMERA;
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_HID) != 0, "multi: has HID");
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_SERIAL) != 0, "multi: has SERIAL");
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_CAMERA) != 0, "multi: has CAMERA");
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, OP_DEVICE_IF_AUDIO), "multi: lacks AUDIO");
}

static void test_device_info_has_interface_null(void) {
    ASSERT_EQ_INT(0, op_device_info_has_interface(NULL, OP_DEVICE_IF_HID), "NULL device returns 0");
}

static void test_device_info_has_interface_combined(void) {
    op_device_info_t device;
    uint32_t combined;
    uint32_t partial;
    uint32_t none;

    op_device_info_init(&device);
    device.interface_flags = OP_DEVICE_IF_HID | OP_DEVICE_IF_SERIAL;

    /* Exact match: both HID and SERIAL are set */
    combined = OP_DEVICE_IF_HID | OP_DEVICE_IF_SERIAL;
    ASSERT_TRUE(op_device_info_has_interface(&device, combined) != 0, "combined match");

    /* Partial match: HID is set but CAMERA is not - returns true because HID is present */
    partial = OP_DEVICE_IF_HID | OP_DEVICE_IF_CAMERA;
    ASSERT_TRUE(op_device_info_has_interface(&device, partial) != 0, "partial match returns 1 (any bit set)");

    /* No match: neither CAMERA nor AUDIO are set */
    none = OP_DEVICE_IF_CAMERA | OP_DEVICE_IF_AUDIO;
    ASSERT_EQ_INT(0, op_device_info_has_interface(&device, none), "no match returns 0");
}

static void test_device_info_has_capability(void) {
    op_device_info_t device;
    op_device_info_init(&device);

    ASSERT_EQ_INT(0, op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_KEYBOARD), "initial lacks keyboard");
    ASSERT_EQ_INT(0, op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE), "initial lacks mouse_abs");

    device.capabilities |= OP_CAPABILITY_INPUT_KEYBOARD;
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "has keyboard");
    ASSERT_EQ_INT(0, op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE), "lacks mouse_abs");

    device.capabilities |= OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE;
    device.capabilities |= OP_CAPABILITY_INPUT_MOUSE_RELATIVE;
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "multi: has keyboard");
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "multi: has mouse_abs");
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) != 0, "multi: has mouse_rel");
}

static void test_device_info_has_capability_null(void) {
    ASSERT_EQ_INT(0, op_device_info_has_capability(NULL, OP_CAPABILITY_INPUT_KEYBOARD), "NULL device returns 0");
}

static void test_device_info_fields(void) {
    op_device_info_t device;
    op_device_info_init(&device);

    strncpy(device.device_id, "test_device_123", sizeof(device.device_id) - 1);
    strncpy(device.profile_id, "profile_456", sizeof(device.profile_id) - 1);
    strncpy(device.port_chain, "1-2.3", sizeof(device.port_chain) - 1);
    device.vendor_id = 0x1234;
    device.product_id = 0x5678;
    device.default_baudrate = 115200;

    ASSERT_EQ_STR("test_device_123", device.device_id, "device_id");
    ASSERT_EQ_STR("profile_456", device.profile_id, "profile_id");
    ASSERT_EQ_STR("1-2.3", device.port_chain, "port_chain");
    ASSERT_EQ_INT(0x1234, device.vendor_id, "vendor_id");
    ASSERT_EQ_INT(0x5678, device.product_id, "product_id");
    ASSERT_EQ_INT(115200, (int)device.default_baudrate, "default_baudrate");
}

int main(void) {
    printf("Running device tests...\n");

    RUN_TEST(test_device_info_init);
    RUN_TEST(test_device_info_init_null);
    RUN_TEST(test_device_info_has_interface);
    RUN_TEST(test_device_info_has_interface_null);
    RUN_TEST(test_device_info_has_interface_combined);
    RUN_TEST(test_device_info_has_capability);
    RUN_TEST(test_device_info_has_capability_null);
    RUN_TEST(test_device_info_fields);

    if (test_failures != 0) {
        fprintf(stderr, "device_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("device_test: all tests passed\n");
    return 0;
}
