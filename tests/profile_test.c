/*
 * profile_test.c - Tests for the device profile module
 *
 * Tests profile registry, find_by_id, match, apply_to_device,
 * has_capability, and device family labels.
 */

#include "openterface/profile.h"
#include "openterface/device.h"
#include "openterface/capability.h"
#include "openterface/chip.h"
#include "test_helpers.h"

int test_failures = 0;

static void test_profile_registry(void) {
    const op_device_profile_t *registry;
    size_t count;

    registry = op_profile_registry();
    ASSERT_TRUE(registry != NULL, "registry is not NULL");

    count = op_profile_registry_count();
    ASSERT_TRUE(count > 0, "count > 0");
    ASSERT_EQ_INT(8, (int)count, "8 device profiles defined");
}

static void test_profile_find_by_id_basic(void) {
    const op_device_profile_t *profile;

    /* Test finding mini-kvm profile */
    profile = op_profile_find_by_id("mini-kvm");
    ASSERT_TRUE(profile != NULL, "mini-kvm found");
    ASSERT_EQ_STR("mini-kvm", profile->profile_id, "mini-kvm profile_id");
    ASSERT_EQ_STR("Mini KVM", profile->display_name, "mini-kvm display_name");
    ASSERT_EQ_INT(OP_DEVICE_FAMILY_MINI_KVM, profile->family, "mini-kvm family");
    ASSERT_EQ_INT(0x1A86, profile->vendor_id, "mini-kvm vendor_id");
    ASSERT_EQ_INT(0x7523, profile->product_id, "mini-kvm product_id");
    ASSERT_EQ_INT(115200, (int)profile->default_baudrate, "mini-kvm baudrate");

    /* Test finding kvm-go profile */
    profile = op_profile_find_by_id("kvm-go");
    ASSERT_TRUE(profile != NULL, "kvm-go found");
    ASSERT_EQ_STR("kvm-go", profile->profile_id, "kvm-go profile_id");
    ASSERT_EQ_INT(OP_DEVICE_FAMILY_KVM_GO, profile->family, "kvm-go family");
    ASSERT_EQ_INT(9600, (int)profile->default_baudrate, "kvm-go baudrate");
}

static void test_profile_find_by_id_not_found(void) {
    const op_device_profile_t *profile;

    profile = op_profile_find_by_id("nonexistent");
    ASSERT_TRUE(profile == NULL, "nonexistent returns NULL");
}

static void test_profile_find_by_id_null_empty(void) {
    ASSERT_TRUE(op_profile_find_by_id(NULL) == NULL, "NULL returns NULL");
    ASSERT_TRUE(op_profile_find_by_id("") == NULL, "empty string returns NULL");
}

static void test_profile_find_by_id_all(void) {
    const char *known_ids[] = {
        "mini-kvm",
        "kvm-go",
        "kvm-go-gen3",
        "kvm-go-v3",
        "keymod",
        "ms2109-video",
        "ms2130s-video",
        "ms2109s-video"
    };
    size_t i;

    for (i = 0; i < sizeof(known_ids) / sizeof(known_ids[0]); i++) {
        const op_device_profile_t *profile = op_profile_find_by_id(known_ids[i]);
        ASSERT_TRUE(profile != NULL, "known profile found");
        ASSERT_EQ_STR(known_ids[i], profile->profile_id, "profile_id matches");
    }
}

static void test_profile_match_basic(void) {
    const op_device_profile_t *profile;

    /* VID=0x1A86, PID=0x7523, IF=SERIAL should match mini-kvm */
    profile = op_profile_match(0x1A86, 0x7523, OP_DEVICE_IF_SERIAL);
    ASSERT_TRUE(profile != NULL, "basic match found");
    ASSERT_EQ_STR("mini-kvm", profile->profile_id, "matches mini-kvm");
}

static void test_profile_match_with_hint(void) {
    const op_device_profile_t *profile;

    /* KVM Go + MS2130S hint -> kvm-go-gen3 */
    profile = op_profile_match_with_hint(
        0x1A86, 0xFE0C, OP_DEVICE_IF_SERIAL, OP_VIDEO_CHIP_MS2130S
    );
    ASSERT_TRUE(profile != NULL, "kvm-go-gen3 found");
    ASSERT_EQ_STR("kvm-go-gen3", profile->profile_id, "matches kvm-go-gen3");
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2130S, profile->chip_hint, "chip_hint is MS2130S");

    /* KVM Go + MS2109S hint -> kvm-go-v3 */
    profile = op_profile_match_with_hint(
        0x1A86, 0xFE0C, OP_DEVICE_IF_SERIAL, OP_VIDEO_CHIP_MS2109S
    );
    ASSERT_TRUE(profile != NULL, "kvm-go-v3 found");
    ASSERT_EQ_STR("kvm-go-v3", profile->profile_id, "matches kvm-go-v3");
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2109S, profile->chip_hint, "chip_hint is MS2109S");
}

static void test_profile_match_no_match(void) {
    const op_device_profile_t *profile;

    profile = op_profile_match(0x9999, 0x9999, 0);
    ASSERT_TRUE(profile == NULL, "no match returns NULL");
}

static void test_profile_match_keymod_fallback(void) {
    const op_device_profile_t *profile;

    /* WCH vendor without matching product_id -> keymod fallback */
    profile = op_profile_match(0x1A86, 0x9999, OP_DEVICE_IF_SERIAL);
    ASSERT_TRUE(profile != NULL, "keymod fallback found");
    ASSERT_EQ_STR("keymod", profile->profile_id, "matches keymod");
}

static void test_profile_match_vendor_filter(void) {
    const op_device_profile_t *profile;

    /* Macrosilicon 534D -> ms2109-video */
    profile = op_profile_match(0x534D, 0x2109, OP_DEVICE_IF_CAMERA);
    ASSERT_TRUE(profile != NULL, "ms2109-video found");
    ASSERT_EQ_STR("ms2109-video", profile->profile_id, "matches ms2109-video");

    /* Macrosilicon 345F -> ms21xx series */
    profile = op_profile_match(0x345F, 0x2132, OP_DEVICE_IF_CAMERA);
    ASSERT_TRUE(profile != NULL, "ms21xx found");
    ASSERT_TRUE(strstr(profile->profile_id, "ms21") != NULL, "profile_id contains ms21");
}

static void test_profile_apply_to_device(void) {
    op_device_info_t device;
    const op_device_profile_t *profile;
    op_status_t status;

    op_device_info_init(&device);

    profile = op_profile_find_by_id("mini-kvm");
    ASSERT_TRUE(profile != NULL, "mini-kvm found");

    status = op_profile_apply_to_device(profile, &device);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "apply returns OK");

    ASSERT_EQ_STR("mini-kvm", device.profile_id, "profile_id set");
    ASSERT_EQ_INT(0x1A86, device.vendor_id, "vendor_id set");
    ASSERT_EQ_INT(0x7523, device.product_id, "product_id set");
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_SERIAL) != 0, "has SERIAL");
    ASSERT_TRUE(op_device_info_has_interface(&device, OP_DEVICE_IF_CAMERA) != 0, "has CAMERA");
    ASSERT_TRUE((device.protocol_flags & OP_PROTOCOL_CH9329) != 0, "has CH9329");
    ASSERT_EQ_INT(115200, (int)device.default_baudrate, "baudrate set");
    ASSERT_EQ_INT(OP_VIDEO_CHIP_MS2109, device.chip_hint, "chip_hint set");
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "has keyboard");
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_VIDEO_CAPTURE) != 0, "has video");
}

static void test_profile_apply_preserves_existing_ids(void) {
    op_device_info_t device;
    const op_device_profile_t *profile;

    op_device_info_init(&device);

    /* Pre-set vendor_id and product_id */
    device.vendor_id = 0x1111;
    device.product_id = 0x2222;

    profile = op_profile_find_by_id("mini-kvm");
    op_profile_apply_to_device(profile, &device);

    /* Existing IDs should NOT be overwritten */
    ASSERT_EQ_INT(0x1111, device.vendor_id, "vendor_id preserved");
    ASSERT_EQ_INT(0x2222, device.product_id, "product_id preserved");
}

static void test_profile_apply_accumulates_capabilities(void) {
    op_device_info_t device;
    const op_device_profile_t *profile;

    op_device_info_init(&device);

    /* Add a capability first */
    device.capabilities |= OP_CAPABILITY_AUDIO_CAPTURE;

    profile = op_profile_find_by_id("mini-kvm");
    op_profile_apply_to_device(profile, &device);

    /* Original capability should be preserved */
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_AUDIO_CAPTURE) != 0, "original audio preserved");
    /* New capability should also be added */
    ASSERT_TRUE(op_device_info_has_capability(&device, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "new keyboard added");
}

static void test_profile_apply_null_safety(void) {
    op_device_info_t device;
    const op_device_profile_t *profile;

    op_device_info_init(&device);

    profile = op_profile_find_by_id("mini-kvm");

    /* NULL profile */
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_profile_apply_to_device(NULL, &device), "NULL profile");

    /* NULL device */
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_profile_apply_to_device(profile, NULL), "NULL device");

    /* Both NULL */
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_profile_apply_to_device(NULL, NULL), "both NULL");
}

static void test_profile_has_capability(void) {
    const op_device_profile_t *profile;

    profile = op_profile_find_by_id("mini-kvm");
    ASSERT_TRUE(profile != NULL, "mini-kvm found");

    /* mini-kvm should have these capabilities */
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "has keyboard");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "has mouse_abs");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_VIDEO_CAPTURE) != 0, "has video");

    /* mini-kvm should NOT have these capabilities */
    ASSERT_EQ_INT(0, op_profile_has_capability(profile, OP_CAPABILITY_VIDEO_INPUT_STATUS), "lacks video_input_status");
    ASSERT_EQ_INT(0, op_profile_has_capability(profile, OP_CAPABILITY_CHIP_REGISTER_ACCESS), "lacks chip_register");
}

static void test_profile_has_capability_kvm_go(void) {
    const op_device_profile_t *profile;

    profile = op_profile_find_by_id("kvm-go");
    ASSERT_TRUE(profile != NULL, "kvm-go found");

    /* kvm-go has more capabilities than mini-kvm */
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_VIDEO_INPUT_STATUS) != 0, "has video_input_status");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_CHIP_REGISTER_ACCESS) != 0, "has chip_register");
    ASSERT_TRUE(op_profile_has_capability(profile, OP_CAPABILITY_FIRMWARE_INFO) != 0, "has firmware_info");
}

static void test_profile_has_capability_null(void) {
    ASSERT_EQ_INT(0, op_profile_has_capability(NULL, OP_CAPABILITY_INPUT_KEYBOARD), "NULL profile returns 0");
}

static void test_device_family_label(void) {
    ASSERT_EQ_STR("mini-kvm", op_device_family_label(OP_DEVICE_FAMILY_MINI_KVM), "mini-kvm label");
    ASSERT_EQ_STR("kvm-go", op_device_family_label(OP_DEVICE_FAMILY_KVM_GO), "kvm-go label");
    ASSERT_EQ_STR("keymod", op_device_family_label(OP_DEVICE_FAMILY_KEYMOD), "keymod label");
    ASSERT_EQ_STR("unknown", op_device_family_label(OP_DEVICE_FAMILY_UNKNOWN), "unknown label");
}

static void test_device_family_label_invalid(void) {
    ASSERT_EQ_STR("unknown", op_device_family_label((op_device_family_t)9999), "invalid enum returns unknown");
}

static void test_profile_capabilities_consistency(void) {
    const op_device_profile_t *registry;
    size_t count;
    size_t i, j;

    op_capability_id_t all_caps[] = {
        OP_CAPABILITY_INPUT_KEYBOARD,
        OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE,
        OP_CAPABILITY_INPUT_MOUSE_RELATIVE,
        OP_CAPABILITY_INPUT_MEDIA,
        OP_CAPABILITY_INPUT_MACRO,
        OP_CAPABILITY_DEVICE_INFO,
        OP_CAPABILITY_DEVICE_LOCK_LEDS,
        OP_CAPABILITY_VIDEO_CAPTURE,
        OP_CAPABILITY_VIDEO_INPUT_STATUS,
        OP_CAPABILITY_USB_ROLE_SWITCH,
        OP_CAPABILITY_SERIAL_BAUD_SWITCH,
        OP_CAPABILITY_CHIP_REGISTER_ACCESS,
        OP_CAPABILITY_FIRMWARE_INFO,
        OP_CAPABILITY_FIRMWARE_UPDATE,
        OP_CAPABILITY_AUDIO_CAPTURE
    };
    size_t caps_count = sizeof(all_caps) / sizeof(all_caps[0]);

    registry = op_profile_registry();
    count = op_profile_registry_count();

    for (i = 0; i < count; i++) {
        const op_device_profile_t *profile = &registry[i];

        for (j = 0; j < caps_count; j++) {
            int has_in_flags = op_capabilities_has(profile->capabilities, all_caps[j]);
            int has_via_api = op_profile_has_capability(profile, all_caps[j]);
            ASSERT_EQ_INT(has_in_flags, has_via_api, "capability consistency");
        }
    }
}

int main(void) {
    printf("Running profile tests...\n");

    RUN_TEST(test_profile_registry);
    RUN_TEST(test_profile_find_by_id_basic);
    RUN_TEST(test_profile_find_by_id_not_found);
    RUN_TEST(test_profile_find_by_id_null_empty);
    RUN_TEST(test_profile_find_by_id_all);
    RUN_TEST(test_profile_match_basic);
    RUN_TEST(test_profile_match_with_hint);
    RUN_TEST(test_profile_match_no_match);
    RUN_TEST(test_profile_match_keymod_fallback);
    RUN_TEST(test_profile_match_vendor_filter);
    RUN_TEST(test_profile_apply_to_device);
    RUN_TEST(test_profile_apply_preserves_existing_ids);
    RUN_TEST(test_profile_apply_accumulates_capabilities);
    RUN_TEST(test_profile_apply_null_safety);
    RUN_TEST(test_profile_has_capability);
    RUN_TEST(test_profile_has_capability_kvm_go);
    RUN_TEST(test_profile_has_capability_null);
    RUN_TEST(test_device_family_label);
    RUN_TEST(test_device_family_label_invalid);
    RUN_TEST(test_profile_capabilities_consistency);

    if (test_failures != 0) {
        fprintf(stderr, "profile_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("profile_test: all tests passed\n");
    return 0;
}
