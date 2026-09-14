/*
 * capability_test.c - Tests for the capability module
 *
 * Tests capability flag operations, labels, and state labels.
 */

#include "openterface/capability.h"
#include "test_helpers.h"

int test_failures = 0;

static void test_capabilities_has_basic(void) {
    op_capability_flags_t caps = OP_CAPABILITY_INPUT_KEYBOARD;

    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "caps has keyboard");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) == 0, "caps lacks mouse_abs");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) == 0, "caps lacks mouse_rel");
}

static void test_capabilities_has_multiple(void) {
    op_capability_flags_t caps = OP_CAPABILITY_INPUT_KEYBOARD
                               | OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE
                               | OP_CAPABILITY_VIDEO_CAPTURE;

    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "caps has keyboard");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "caps has mouse_abs");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_VIDEO_CAPTURE) != 0, "caps has video");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) == 0, "caps lacks mouse_rel");
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_AUDIO_CAPTURE) == 0, "caps lacks audio");
}

static void test_capabilities_has_none(void) {
    op_capability_flags_t caps = OP_CAPABILITY_INPUT_KEYBOARD | OP_CAPABILITY_VIDEO_CAPTURE;

    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_NONE) == 0, "NONE returns 0");
    ASSERT_TRUE(op_capabilities_has(0, OP_CAPABILITY_INPUT_KEYBOARD) == 0, "empty flags returns 0");
    ASSERT_TRUE(op_capabilities_has(0, OP_CAPABILITY_NONE) == 0, "NONE in empty flags returns 0");
}

static void test_capabilities_has_all_individual(void) {
    op_capability_flags_t caps = 0;

    caps |= OP_CAPABILITY_INPUT_KEYBOARD;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "has keyboard");

    caps |= OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "has mouse_abs");

    caps |= OP_CAPABILITY_INPUT_MOUSE_RELATIVE;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) != 0, "has mouse_rel");

    caps |= OP_CAPABILITY_INPUT_MEDIA;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MEDIA) != 0, "has media");

    caps |= OP_CAPABILITY_INPUT_MACRO;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_INPUT_MACRO) != 0, "has macro");

    caps |= OP_CAPABILITY_DEVICE_INFO;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_DEVICE_INFO) != 0, "has device_info");

    caps |= OP_CAPABILITY_DEVICE_LOCK_LEDS;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_DEVICE_LOCK_LEDS) != 0, "has lock_leds");

    caps |= OP_CAPABILITY_VIDEO_CAPTURE;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_VIDEO_CAPTURE) != 0, "has video");

    caps |= OP_CAPABILITY_VIDEO_INPUT_STATUS;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_VIDEO_INPUT_STATUS) != 0, "has video_input_status");

    caps |= OP_CAPABILITY_USB_ROLE_SWITCH;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_USB_ROLE_SWITCH) != 0, "has role_switch");

    caps |= OP_CAPABILITY_SERIAL_BAUD_SWITCH;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_SERIAL_BAUD_SWITCH) != 0, "has baud_switch");

    caps |= OP_CAPABILITY_CHIP_REGISTER_ACCESS;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_CHIP_REGISTER_ACCESS) != 0, "has chip_register");

    caps |= OP_CAPABILITY_FIRMWARE_INFO;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_FIRMWARE_INFO) != 0, "has firmware_info");

    caps |= OP_CAPABILITY_FIRMWARE_UPDATE;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_FIRMWARE_UPDATE) != 0, "has firmware_update");

    caps |= OP_CAPABILITY_AUDIO_CAPTURE;
    ASSERT_TRUE(op_capabilities_has(caps, OP_CAPABILITY_AUDIO_CAPTURE) != 0, "has audio");
}

static void test_capabilities_input_basic(void) {
    op_capability_flags_t basic = op_capabilities_input_basic();

    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "basic has keyboard");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "basic has mouse_abs");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) != 0, "basic has mouse_rel");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_INPUT_MEDIA) != 0, "basic has media");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_INPUT_MACRO) != 0, "basic has macro");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_DEVICE_INFO) != 0, "basic has device_info");

    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_DEVICE_LOCK_LEDS) == 0, "basic lacks lock_leds");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_VIDEO_CAPTURE) == 0, "basic lacks video");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_VIDEO_INPUT_STATUS) == 0, "basic lacks video_input_status");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_USB_ROLE_SWITCH) == 0, "basic lacks role_switch");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_SERIAL_BAUD_SWITCH) == 0, "basic lacks baud_switch");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_CHIP_REGISTER_ACCESS) == 0, "basic lacks chip_register");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_FIRMWARE_INFO) == 0, "basic lacks firmware_info");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_FIRMWARE_UPDATE) == 0, "basic lacks firmware_update");
    ASSERT_TRUE(op_capabilities_has(basic, OP_CAPABILITY_AUDIO_CAPTURE) == 0, "basic lacks audio");
}

static void test_capability_label_all(void) {
    ASSERT_EQ_STR("input.keyboard", op_capability_label(OP_CAPABILITY_INPUT_KEYBOARD), "keyboard label");
    ASSERT_EQ_STR("input.mouse.absolute", op_capability_label(OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE), "mouse_abs label");
    ASSERT_EQ_STR("input.mouse.relative", op_capability_label(OP_CAPABILITY_INPUT_MOUSE_RELATIVE), "mouse_rel label");
    ASSERT_EQ_STR("input.media", op_capability_label(OP_CAPABILITY_INPUT_MEDIA), "media label");
    ASSERT_EQ_STR("input.macro", op_capability_label(OP_CAPABILITY_INPUT_MACRO), "macro label");
    ASSERT_EQ_STR("device.info", op_capability_label(OP_CAPABILITY_DEVICE_INFO), "device_info label");
    ASSERT_EQ_STR("device.lock_leds", op_capability_label(OP_CAPABILITY_DEVICE_LOCK_LEDS), "lock_leds label");
    ASSERT_EQ_STR("video.capture", op_capability_label(OP_CAPABILITY_VIDEO_CAPTURE), "video label");
    ASSERT_EQ_STR("video.input_status", op_capability_label(OP_CAPABILITY_VIDEO_INPUT_STATUS), "video_input label");
    ASSERT_EQ_STR("usb.role_switch", op_capability_label(OP_CAPABILITY_USB_ROLE_SWITCH), "role_switch label");
    ASSERT_EQ_STR("serial.baud_switch", op_capability_label(OP_CAPABILITY_SERIAL_BAUD_SWITCH), "baud_switch label");
    ASSERT_EQ_STR("chip.register_access", op_capability_label(OP_CAPABILITY_CHIP_REGISTER_ACCESS), "chip_register label");
    ASSERT_EQ_STR("firmware.info", op_capability_label(OP_CAPABILITY_FIRMWARE_INFO), "firmware_info label");
    ASSERT_EQ_STR("firmware.update", op_capability_label(OP_CAPABILITY_FIRMWARE_UPDATE), "firmware_update label");
    ASSERT_EQ_STR("audio.capture", op_capability_label(OP_CAPABILITY_AUDIO_CAPTURE), "audio label");
}

static void test_capability_label_unknown(void) {
    ASSERT_EQ_STR("unknown", op_capability_label(OP_CAPABILITY_NONE), "NONE is unknown");
    ASSERT_EQ_STR("unknown", op_capability_label((op_capability_id_t)9999), "invalid is unknown");
}

static void test_capability_state_label_all(void) {
    ASSERT_EQ_STR("unsupported", op_capability_state_label(OP_CAPABILITY_STATE_UNSUPPORTED), "unsupported label");
    ASSERT_EQ_STR("available", op_capability_state_label(OP_CAPABILITY_STATE_AVAILABLE), "available label");
    ASSERT_EQ_STR("permission_required", op_capability_state_label(OP_CAPABILITY_STATE_PERMISSION_REQUIRED), "permission label");
    ASSERT_EQ_STR("disconnected", op_capability_state_label(OP_CAPABILITY_STATE_DISCONNECTED), "disconnected label");
    ASSERT_EQ_STR("busy", op_capability_state_label(OP_CAPABILITY_STATE_BUSY), "busy label");
    ASSERT_EQ_STR("degraded", op_capability_state_label(OP_CAPABILITY_STATE_DEGRADED), "degraded label");
    ASSERT_EQ_STR("requires_local_service", op_capability_state_label(OP_CAPABILITY_STATE_REQUIRES_LOCAL_SERVICE), "local_service label");
    ASSERT_EQ_STR("requires_firmware", op_capability_state_label(OP_CAPABILITY_STATE_REQUIRES_FIRMWARE), "firmware label");
}

static void test_capability_state_label_unknown(void) {
    ASSERT_EQ_STR("unsupported", op_capability_state_label((op_capability_state_t)9999), "invalid state is unsupported");
}

static void test_capability_flags_are_unique(void) {
    op_capability_id_t capabilities[] = {
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

    int count = sizeof(capabilities) / sizeof(capabilities[0]);
    int i, j;

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            ASSERT_EQ_INT(0, (int)(capabilities[i] & capabilities[j]), "flags are unique");
        }
    }
}

static void test_capability_flags_combination(void) {
    op_capability_flags_t caps1 = OP_CAPABILITY_INPUT_KEYBOARD | OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE;
    op_capability_flags_t caps2 = OP_CAPABILITY_VIDEO_CAPTURE | OP_CAPABILITY_AUDIO_CAPTURE;
    op_capability_flags_t combined = caps1 | caps2;

    ASSERT_TRUE(op_capabilities_has(combined, OP_CAPABILITY_INPUT_KEYBOARD) != 0, "combined has keyboard");
    ASSERT_TRUE(op_capabilities_has(combined, OP_CAPABILITY_INPUT_MOUSE_ABSOLUTE) != 0, "combined has mouse_abs");
    ASSERT_TRUE(op_capabilities_has(combined, OP_CAPABILITY_VIDEO_CAPTURE) != 0, "combined has video");
    ASSERT_TRUE(op_capabilities_has(combined, OP_CAPABILITY_AUDIO_CAPTURE) != 0, "combined has audio");
    ASSERT_TRUE(op_capabilities_has(combined, OP_CAPABILITY_INPUT_MOUSE_RELATIVE) == 0, "combined lacks mouse_rel");
}

int main(void) {
    printf("Running capability tests...\n");

    RUN_TEST(test_capabilities_has_basic);
    RUN_TEST(test_capabilities_has_multiple);
    RUN_TEST(test_capabilities_has_none);
    RUN_TEST(test_capabilities_has_all_individual);
    RUN_TEST(test_capabilities_input_basic);
    RUN_TEST(test_capability_label_all);
    RUN_TEST(test_capability_label_unknown);
    RUN_TEST(test_capability_state_label_all);
    RUN_TEST(test_capability_state_label_unknown);
    RUN_TEST(test_capability_flags_are_unique);
    RUN_TEST(test_capability_flags_combination);

    if (test_failures != 0) {
        fprintf(stderr, "capability_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("capability_test: all tests passed\n");
    return 0;
}
