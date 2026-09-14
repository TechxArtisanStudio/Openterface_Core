/*
 * protocol_ch9329_test.c - Tests for the CH9329 protocol module
 *
 * Tests USB switch packet build/parse, constants, checksums,
 * and wrapper functions.
 */

#include "openterface/protocol_ch9329.h"
#include "openterface/input.h"
#include "test_helpers.h"

int test_failures = 0;

static void test_constants(void) {
    ASSERT_EQ_INT(0x57, OP_CH9329_HEADER_0, "HEADER_0");
    ASSERT_EQ_INT(0xAB, OP_CH9329_HEADER_1, "HEADER_1");
    ASSERT_EQ_INT(0x00, OP_CH9329_ADDR_DEFAULT, "ADDR_DEFAULT");
    ASSERT_EQ_INT(0x17, OP_CH9329_CMD_USB_SWITCH, "CMD_USB_SWITCH");
    ASSERT_EQ_INT(0x97, OP_CH9329_RESP_USB_SWITCH, "RESP_USB_SWITCH");
    ASSERT_EQ_INT(0x00, OP_CH9329_USB_SWITCH_HOST, "SWITCH_HOST");
    ASSERT_EQ_INT(0x01, OP_CH9329_USB_SWITCH_TARGET, "SWITCH_TARGET");
    ASSERT_EQ_INT(0x03, OP_CH9329_USB_SWITCH_QUERY, "SWITCH_QUERY");
    ASSERT_EQ_INT(11, OP_CH9329_PKT_USB_SWITCH_SIZE, "PKT_USB_SWITCH_SIZE");
    ASSERT_EQ_INT(7, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE, "PKT_USB_SWITCH_RESPONSE_SIZE");
}

static void test_build_usb_switch_packet_host(void) {
    uint8_t packet[OP_CH9329_PKT_USB_SWITCH_SIZE];
    int len;

    len = op_ch9329_build_usb_switch_packet(packet, OP_CH9329_USB_SWITCH_HOST);
    ASSERT_EQ_INT(OP_CH9329_PKT_USB_SWITCH_SIZE, len, "packet length");
    ASSERT_EQ_INT(OP_CH9329_HEADER_0, packet[0], "header[0]");
    ASSERT_EQ_INT(OP_CH9329_HEADER_1, packet[1], "header[1]");
    ASSERT_EQ_INT(OP_CH9329_ADDR_DEFAULT, packet[2], "addr");
    ASSERT_EQ_INT(OP_CH9329_CMD_USB_SWITCH, packet[3], "cmd");
    ASSERT_EQ_INT(0x05, packet[4], "data length");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_HOST, packet[9], "data = HOST");
    ASSERT_EQ_INT(op_ch9329_checksum(packet, OP_CH9329_PKT_USB_SWITCH_SIZE), packet[10], "checksum");
}

static void test_build_usb_switch_packet_target(void) {
    uint8_t packet[OP_CH9329_PKT_USB_SWITCH_SIZE];
    int len;

    len = op_ch9329_build_usb_switch_packet(packet, OP_CH9329_USB_SWITCH_TARGET);
    ASSERT_EQ_INT(OP_CH9329_PKT_USB_SWITCH_SIZE, len, "packet length");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_TARGET, packet[9], "data = TARGET");
    ASSERT_EQ_INT(op_ch9329_checksum(packet, OP_CH9329_PKT_USB_SWITCH_SIZE), packet[10], "checksum");
}

static void test_build_usb_switch_packet_query(void) {
    uint8_t packet[OP_CH9329_PKT_USB_SWITCH_SIZE];
    int len;

    len = op_ch9329_build_usb_switch_packet(packet, OP_CH9329_USB_SWITCH_QUERY);
    ASSERT_EQ_INT(OP_CH9329_PKT_USB_SWITCH_SIZE, len, "packet length");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_QUERY, packet[9], "data = QUERY");
}

static void test_build_usb_switch_packet_null(void) {
    int len;

    len = op_ch9329_build_usb_switch_packet(NULL, OP_CH9329_USB_SWITCH_HOST);
    ASSERT_EQ_INT(0, len, "NULL packet returns 0");
}

static void test_parse_usb_switch_response_host(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        OP_CH9329_USB_SWITCH_HOST,
        0x00
    };
    uint8_t status;
    op_status_t result;

    response[6] = op_ch9329_checksum(response, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE);

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "parse returns OK");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_HOST, status, "status = HOST");
}

static void test_parse_usb_switch_response_target(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        OP_CH9329_USB_SWITCH_TARGET,
        0x00
    };
    uint8_t status;
    op_status_t result;

    response[6] = op_ch9329_checksum(response, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE);

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_OK, result, "parse returns OK");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_TARGET, status, "status = TARGET");
}

static void test_parse_usb_switch_response_too_short(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01
    };
    uint8_t status;
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, result, "short packet returns IO_ERROR");
}

static void test_parse_usb_switch_response_bad_header(void) {
    uint8_t response[] = {
        0x00,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        OP_CH9329_USB_SWITCH_HOST,
        0x00
    };
    uint8_t status;
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, result, "bad header returns IO_ERROR");
}

static void test_parse_usb_switch_response_bad_command(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        0x00,
        0x01,
        OP_CH9329_USB_SWITCH_HOST,
        0x00
    };
    uint8_t status;
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, result, "bad command returns IO_ERROR");
}

static void test_parse_usb_switch_response_bad_checksum(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        OP_CH9329_USB_SWITCH_HOST,
        0xFF
    };
    uint8_t status;
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, result, "bad checksum returns IO_ERROR");
}

static void test_parse_usb_switch_response_invalid_status(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        0x05,
        0x00
    };
    uint8_t status;
    op_status_t result;

    response[6] = op_ch9329_checksum(response, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE);

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), &status);
    ASSERT_EQ_INT(OP_STATUS_NOT_SUPPORTED, result, "invalid status returns NOT_SUPPORTED");
}

static void test_parse_usb_switch_response_null_packet(void) {
    uint8_t status;
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(NULL, 7, &status);
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, result, "NULL packet returns INVALID_ARGUMENT");
}

static void test_parse_usb_switch_response_null_status(void) {
    uint8_t response[] = {
        OP_CH9329_HEADER_0,
        OP_CH9329_HEADER_1,
        OP_CH9329_ADDR_DEFAULT,
        OP_CH9329_RESP_USB_SWITCH,
        0x01,
        OP_CH9329_USB_SWITCH_HOST,
        0x00
    };
    op_status_t result;

    result = op_ch9329_parse_usb_switch_response(response, sizeof(response), NULL);
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, result, "NULL status returns INVALID_ARGUMENT");
}

static void test_wrapper_functions(void) {
    uint8_t keyboard_pkt[OP_CH9329_PKT_KEYBOARD_SIZE];
    uint8_t mouse_rel_pkt[OP_CH9329_PKT_MOUSE_REL_SIZE];
    uint8_t mouse_abs_pkt[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    uint8_t press_release[2 * OP_CH9329_PKT_KEYBOARD_SIZE];
    uint8_t keys[] = {0x04, 0x05};
    int len;

    len = op_ch9329_build_keyboard_packet(keyboard_pkt, 0x00, keys, 2);
    ASSERT_EQ_INT(OP_CH9329_PKT_KEYBOARD_SIZE, len, "keyboard packet length");

    len = op_ch9329_build_mouse_rel_packet(mouse_rel_pkt, 0x01, 10, 20, 0);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_REL_SIZE, len, "mouse_rel packet length");

    len = op_ch9329_build_mouse_abs_packet(mouse_abs_pkt, 0x01, 1000, 2000, 0);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_ABS_SIZE, len, "mouse_abs packet length");

    len = op_ch9329_build_press_release_packets(press_release, 0x00, 0x04);
    ASSERT_EQ_INT(2 * OP_CH9329_PKT_KEYBOARD_SIZE, len, "press_release packet length");
}

int main(void) {
    printf("Running protocol_ch9329 tests...\n");

    RUN_TEST(test_constants);
    RUN_TEST(test_build_usb_switch_packet_host);
    RUN_TEST(test_build_usb_switch_packet_target);
    RUN_TEST(test_build_usb_switch_packet_query);
    RUN_TEST(test_build_usb_switch_packet_null);
    RUN_TEST(test_parse_usb_switch_response_host);
    RUN_TEST(test_parse_usb_switch_response_target);
    RUN_TEST(test_parse_usb_switch_response_too_short);
    RUN_TEST(test_parse_usb_switch_response_bad_header);
    RUN_TEST(test_parse_usb_switch_response_bad_command);
    RUN_TEST(test_parse_usb_switch_response_bad_checksum);
    RUN_TEST(test_parse_usb_switch_response_invalid_status);
    RUN_TEST(test_parse_usb_switch_response_null_packet);
    RUN_TEST(test_parse_usb_switch_response_null_status);
    RUN_TEST(test_wrapper_functions);

    if (test_failures != 0) {
        fprintf(stderr, "protocol_ch9329_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("protocol_ch9329_test: all tests passed\n");
    return 0;
}
