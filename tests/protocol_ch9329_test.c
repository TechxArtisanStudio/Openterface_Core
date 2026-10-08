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

    len = op_ch9329_build_keyboard_packet(keyboard_pkt, 0x00, keys, 2, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(OP_CH9329_PKT_KEYBOARD_SIZE, len, "keyboard packet length");

    len = op_ch9329_build_mouse_rel_packet(mouse_rel_pkt, 0x01, 10, 20, 0);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_REL_SIZE, len, "mouse_rel packet length");

    len = op_ch9329_build_mouse_abs_packet(mouse_abs_pkt, 0x01, 1000, 2000, 0);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_ABS_SIZE, len, "mouse_abs packet length");

    len = op_ch9329_build_press_release_packets(press_release, 0x00, 0x04, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(2 * OP_CH9329_PKT_KEYBOARD_SIZE, len, "press_release packet length");
}

/* ── op_ch9329_parse_packet tests ────────────────────────────────────── */

static void test_parse_keyboard_packet(void) {
    uint8_t raw[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;
    uint8_t keys[] = {0x04, 0x05, 0x06};

    int len = op_ch9329_build_keyboard_packet(raw, OP_INPUT_MOD_SHIFT, keys, 3, OP_INPUT_KB_FLAG_NONE);
    ASSERT_EQ_INT(OP_CH9329_PKT_KEYBOARD_SIZE, len, "built packet length");

    status = op_ch9329_parse_packet(raw, len, &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_TRUE(parsed.valid, "packet is valid");
    ASSERT_EQ_INT(OP_CH9329_HEADER_0, parsed.header[0], "header[0]");
    ASSERT_EQ_INT(OP_CH9329_HEADER_1, parsed.header[1], "header[1]");
    ASSERT_EQ_INT(OP_CH9329_ADDR_DEFAULT, parsed.header[2], "header[2] addr");
    ASSERT_EQ_INT(OP_CH9329_CMD_KEYBOARD, parsed.header[3], "header[3] cmd");
    ASSERT_EQ_INT(0x08, parsed.header[4], "header[4] data_len");
    ASSERT_EQ_INT(0x08, parsed.data_len, "parsed data_len");
    ASSERT_EQ_INT(OP_INPUT_MOD_SHIFT, parsed.data[0], "data[0] modifiers");
    ASSERT_EQ_INT(0x04, parsed.data[2], "data[2] key0");
    ASSERT_EQ_INT(0x05, parsed.data[3], "data[3] key1");
    ASSERT_EQ_INT(0x06, parsed.data[4], "data[4] key2");
    ASSERT_EQ_INT(raw[OP_CH9329_PKT_KEYBOARD_SIZE - 1], parsed.checksum, "checksum matches");
    ASSERT_EQ_INT(parsed.checksum_calc, parsed.checksum, "checksum calc matches");
}

static void test_parse_mouse_rel_packet(void) {
    uint8_t raw[OP_CH9329_PKT_MOUSE_REL_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    int len = op_ch9329_build_mouse_rel_packet(raw, OP_INPUT_MS_BTN_LEFT, 10, -20, 1);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_REL_SIZE, len, "built packet length");

    status = op_ch9329_parse_packet(raw, len, &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_TRUE(parsed.valid, "packet is valid");
    ASSERT_EQ_INT(OP_CH9329_CMD_MOUSE_REL, parsed.header[3], "cmd = MS_REL");
    ASSERT_EQ_INT(0x05, parsed.data_len, "data_len = 5");
    ASSERT_EQ_INT(OP_INPUT_MS_BTN_LEFT, parsed.data[1], "buttons");
}

static void test_parse_mouse_abs_packet(void) {
    uint8_t raw[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    int len = op_ch9329_build_mouse_abs_packet(raw, OP_INPUT_MS_BTN_RIGHT, 1000, 2000, 0);
    ASSERT_EQ_INT(OP_CH9329_PKT_MOUSE_ABS_SIZE, len, "built packet length");

    status = op_ch9329_parse_packet(raw, len, &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_TRUE(parsed.valid, "packet is valid");
    ASSERT_EQ_INT(OP_CH9329_CMD_MOUSE_ABS, parsed.header[3], "cmd = MS_ABS");
    ASSERT_EQ_INT(0x07, parsed.data_len, "data_len = 7");
}

static void test_parse_usb_switch_response_packet(void) {
    uint8_t raw[OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    raw[0] = OP_CH9329_HEADER_0;
    raw[1] = OP_CH9329_HEADER_1;
    raw[2] = OP_CH9329_ADDR_DEFAULT;
    raw[3] = OP_CH9329_RESP_USB_SWITCH;
    raw[4] = 0x01;
    raw[5] = OP_CH9329_USB_SWITCH_HOST;
    raw[6] = op_ch9329_checksum(raw, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE);

    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE, &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_TRUE(parsed.valid, "packet is valid");
    ASSERT_EQ_INT(OP_CH9329_RESP_USB_SWITCH, parsed.header[3], "cmd = RESP_USB_SWITCH");
    ASSERT_EQ_INT(0x01, parsed.data_len, "data_len = 1");
    ASSERT_EQ_INT(OP_CH9329_USB_SWITCH_HOST, parsed.data[0], "data = HOST");
}

static void test_parse_packet_null_raw(void) {
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    status = op_ch9329_parse_packet(NULL, 14, &parsed);
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, status, "NULL raw returns INVALID_ARGUMENT");
}

static void test_parse_packet_null_out(void) {
    uint8_t raw[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_status_t status;

    op_ch9329_build_keyboard_packet(raw, 0, NULL, 0, OP_INPUT_KB_FLAG_NONE);
    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_KEYBOARD_SIZE, NULL);
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, status, "NULL out returns INVALID_ARGUMENT");
}

static void test_parse_packet_too_short(void) {
    uint8_t raw[5] = {0x57, 0xAB, 0x00, 0x02, 0x08};
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    status = op_ch9329_parse_packet(raw, 5, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "5-byte packet returns IO_ERROR");

    status = op_ch9329_parse_packet(raw, 0, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "0-byte packet returns IO_ERROR");

    status = op_ch9329_parse_packet(raw, -1, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "negative len returns IO_ERROR");
}

static void test_parse_packet_bad_header(void) {
    uint8_t raw[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    op_ch9329_build_keyboard_packet(raw, 0, NULL, 0, OP_INPUT_KB_FLAG_NONE);

    raw[0] = 0x00;
    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_KEYBOARD_SIZE, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "bad header[0] returns IO_ERROR");

    raw[0] = OP_CH9329_HEADER_0;
    raw[1] = 0xFF;
    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_KEYBOARD_SIZE, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "bad header[1] returns IO_ERROR");
}

static void test_parse_packet_bad_checksum(void) {
    uint8_t raw[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    op_ch9329_build_keyboard_packet(raw, 0, NULL, 0, OP_INPUT_KB_FLAG_NONE);
    raw[OP_CH9329_PKT_KEYBOARD_SIZE - 1] = 0xFF; /* corrupt checksum */

    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_KEYBOARD_SIZE, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "bad checksum returns IO_ERROR");
    ASSERT_TRUE(!parsed.valid, "packet is marked invalid");
    ASSERT_EQ_INT(0xFF, parsed.checksum, "checksum = received corrupt value");
    ASSERT_NEQ(parsed.checksum, parsed.checksum_calc, "checksum != checksum_calc");
}

static void test_parse_packet_len_mismatch(void) {
    uint8_t raw[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_parsed_packet_t parsed;
    op_status_t status;

    op_ch9329_build_keyboard_packet(raw, 0, NULL, 0, OP_INPUT_KB_FLAG_NONE);

    /* Pass a shorter length than the header declares */
    status = op_ch9329_parse_packet(raw, 10, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "truncated packet returns IO_ERROR");

    /* Pass a longer length than expected */
    status = op_ch9329_parse_packet(raw, OP_CH9329_PKT_KEYBOARD_SIZE + 1, &parsed);
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, status, "overlong packet returns IO_ERROR");
}

static void test_parse_packet_roundtrip_all_types(void) {
    uint8_t keyboard[OP_CH9329_PKT_KEYBOARD_SIZE];
    uint8_t mouse_rel[OP_CH9329_PKT_MOUSE_REL_SIZE];
    uint8_t mouse_abs[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    uint8_t usb_resp[OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE];
    op_ch9329_parsed_packet_t parsed;

    uint8_t keys[] = {0x04, 0x05};
    int klen = op_ch9329_build_keyboard_packet(keyboard, OP_INPUT_MOD_CTRL, keys, 2, OP_INPUT_KB_FLAG_NONE);
    int mrlen = op_ch9329_build_mouse_rel_packet(mouse_rel, OP_INPUT_MS_BTN_NONE, 5, -5, 0);
    int malen = op_ch9329_build_mouse_abs_packet(mouse_abs, OP_INPUT_MS_BTN_MIDDLE, 320, 240, -1);
    int uslen = OP_CH9329_PKT_USB_SWITCH_RESPONSE_SIZE;

    usb_resp[0] = OP_CH9329_HEADER_0;
    usb_resp[1] = OP_CH9329_HEADER_1;
    usb_resp[2] = OP_CH9329_ADDR_DEFAULT;
    usb_resp[3] = OP_CH9329_RESP_USB_SWITCH;
    usb_resp[4] = 0x01;
    usb_resp[5] = OP_CH9329_USB_SWITCH_TARGET;
    usb_resp[6] = op_ch9329_checksum(usb_resp, uslen);

    ASSERT_EQ_INT(OP_STATUS_OK, op_ch9329_parse_packet(keyboard, klen, &parsed), "keyboard roundtrip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_ch9329_parse_packet(mouse_rel, mrlen, &parsed), "mouse_rel roundtrip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_ch9329_parse_packet(mouse_abs, malen, &parsed), "mouse_abs roundtrip");
    ASSERT_EQ_INT(OP_STATUS_OK, op_ch9329_parse_packet(usb_resp, uslen, &parsed), "usb_resp roundtrip");
}

/* ── Specific response parser tests ────────────────────────────────────── */

static void test_parse_keyboard_response(void) {
    /* Build a keyboard response: same structure as keyboard packet but cmd = 0x82 */
    uint8_t response[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_keyboard_response_t parsed;
    op_status_t status;

    response[0] = OP_CH9329_HEADER_0;
    response[1] = OP_CH9329_HEADER_1;
    response[2] = OP_CH9329_ADDR_DEFAULT;
    response[3] = OP_CH9329_RESP_KEYBOARD;  /* 0x82 */
    response[4] = 0x08;                      /* data length */
    response[5] = OP_INPUT_MOD_SHIFT;        /* modifiers */
    response[6] = 0x00;                      /* reserved */
    response[7] = 0x04;                      /* key 'A' */
    response[8] = 0x05;                      /* key 'B' */
    response[9] = 0x00;
    response[10] = 0x00;
    response[11] = 0x00;
    response[12] = 0x00;
    response[13] = op_ch9329_checksum(response, OP_CH9329_PKT_KEYBOARD_SIZE);

    status = op_ch9329_parse_keyboard_response(response, sizeof(response), &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_EQ_INT(OP_INPUT_MOD_SHIFT, parsed.modifiers, "modifiers = SHIFT");
    ASSERT_EQ_INT(0x00, parsed.reserved, "reserved = 0");
    ASSERT_EQ_INT(0x04, parsed.keys[0], "key[0] = A");
    ASSERT_EQ_INT(0x05, parsed.keys[1], "key[1] = B");
    ASSERT_EQ_INT(0x00, parsed.keys[2], "key[2] = empty");
}

static void test_parse_keyboard_response_null(void) {
    uint8_t response[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_keyboard_response_t parsed;

    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_keyboard_response(NULL, sizeof(response), &parsed),
                  "NULL packet");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_keyboard_response(response, sizeof(response), NULL),
                  "NULL out");
}

static void test_parse_keyboard_response_bad_cmd(void) {
    uint8_t response[OP_CH9329_PKT_KEYBOARD_SIZE];
    op_ch9329_keyboard_response_t parsed;

    response[0] = OP_CH9329_HEADER_0;
    response[1] = OP_CH9329_HEADER_1;
    response[2] = OP_CH9329_ADDR_DEFAULT;
    response[3] = 0x02;  /* wrong: should be 0x82 */
    response[4] = 0x08;
    memset(response + 5, 0, 9);
    response[13] = op_ch9329_checksum(response, OP_CH9329_PKT_KEYBOARD_SIZE);

    ASSERT_EQ_INT(OP_STATUS_IO_ERROR,
                  op_ch9329_parse_keyboard_response(response, sizeof(response), &parsed),
                  "bad cmd returns IO_ERROR");
}

static void test_parse_mouse_rel_response(void) {
    uint8_t response[OP_CH9329_PKT_MOUSE_REL_SIZE];
    op_ch9329_mouse_rel_response_t parsed;
    op_status_t status;

    response[0] = OP_CH9329_HEADER_0;
    response[1] = OP_CH9329_HEADER_1;
    response[2] = OP_CH9329_ADDR_DEFAULT;
    response[3] = OP_CH9329_RESP_MOUSE_REL;  /* 0x85 */
    response[4] = 0x05;                       /* data length */
    response[5] = 0x01;                       /* mode = relative */
    response[6] = OP_INPUT_MS_BTN_LEFT;       /* buttons */
    response[7] = 10;                         /* dx */
    response[8] = (uint8_t)(-20);             /* dy = -20 */
    response[9] = 1;                          /* wheel */
    response[10] = op_ch9329_checksum(response, OP_CH9329_PKT_MOUSE_REL_SIZE);

    status = op_ch9329_parse_mouse_rel_response(response, sizeof(response), &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_EQ_INT(0x01, parsed.mode, "mode = relative");
    ASSERT_EQ_INT(OP_INPUT_MS_BTN_LEFT, parsed.buttons, "buttons = LEFT");
    ASSERT_EQ_INT(10, parsed.dx, "dx = 10");
    ASSERT_EQ_INT(-20, parsed.dy, "dy = -20");
    ASSERT_EQ_INT(1, parsed.wheel, "wheel = 1");
}

static void test_parse_mouse_rel_response_null(void) {
    uint8_t response[OP_CH9329_PKT_MOUSE_REL_SIZE];
    op_ch9329_mouse_rel_response_t parsed;

    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_mouse_rel_response(NULL, sizeof(response), &parsed),
                  "NULL packet");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_mouse_rel_response(response, sizeof(response), NULL),
                  "NULL out");
}

static void test_parse_mouse_abs_response(void) {
    uint8_t response[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    op_ch9329_mouse_abs_response_t parsed;
    op_status_t status;
    uint16_t x = 1000, y = 2000;

    response[0] = OP_CH9329_HEADER_0;
    response[1] = OP_CH9329_HEADER_1;
    response[2] = OP_CH9329_ADDR_DEFAULT;
    response[3] = OP_CH9329_RESP_MOUSE_ABS;  /* 0x84 */
    response[4] = 0x07;                       /* data length */
    response[5] = 0x02;                       /* mode = absolute */
    response[6] = OP_INPUT_MS_BTN_RIGHT;      /* buttons */
    response[7] = x & 0xFF;                   /* x low */
    response[8] = (x >> 8) & 0xFF;            /* x high */
    response[9] = y & 0xFF;                   /* y low */
    response[10] = (y >> 8) & 0xFF;           /* y high */
    response[11] = (uint8_t)(-1);             /* wheel = -1 */
    response[12] = op_ch9329_checksum(response, OP_CH9329_PKT_MOUSE_ABS_SIZE);

    status = op_ch9329_parse_mouse_abs_response(response, sizeof(response), &parsed);
    ASSERT_EQ_INT(OP_STATUS_OK, status, "parse returns OK");
    ASSERT_EQ_INT(0x02, parsed.mode, "mode = absolute");
    ASSERT_EQ_INT(OP_INPUT_MS_BTN_RIGHT, parsed.buttons, "buttons = RIGHT");
    ASSERT_EQ_INT(1000, parsed.x, "x = 1000");
    ASSERT_EQ_INT(2000, parsed.y, "y = 2000");
    ASSERT_EQ_INT(-1, parsed.wheel, "wheel = -1");
}

static void test_parse_mouse_abs_response_null(void) {
    uint8_t response[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    op_ch9329_mouse_abs_response_t parsed;

    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_mouse_abs_response(NULL, sizeof(response), &parsed),
                  "NULL packet");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT,
                  op_ch9329_parse_mouse_abs_response(response, sizeof(response), NULL),
                  "NULL out");
}

static void test_parse_response_bad_checksum(void) {
    uint8_t kb_resp[OP_CH9329_PKT_KEYBOARD_SIZE];
    uint8_t ms_rel_resp[OP_CH9329_PKT_MOUSE_REL_SIZE];
    uint8_t ms_abs_resp[OP_CH9329_PKT_MOUSE_ABS_SIZE];
    op_ch9329_keyboard_response_t kb_parsed;
    op_ch9329_mouse_rel_response_t ms_rel_parsed;
    op_ch9329_mouse_abs_response_t ms_abs_parsed;

    /* Keyboard response with bad checksum */
    kb_resp[0] = OP_CH9329_HEADER_0;
    kb_resp[1] = OP_CH9329_HEADER_1;
    kb_resp[2] = OP_CH9329_ADDR_DEFAULT;
    kb_resp[3] = OP_CH9329_RESP_KEYBOARD;
    kb_resp[4] = 0x08;
    memset(kb_resp + 5, 0, 8);
    kb_resp[13] = 0xFF;  /* bad checksum */
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR,
                  op_ch9329_parse_keyboard_response(kb_resp, sizeof(kb_resp), &kb_parsed),
                  "keyboard bad checksum");

    /* Mouse rel response with bad checksum */
    ms_rel_resp[0] = OP_CH9329_HEADER_0;
    ms_rel_resp[1] = OP_CH9329_HEADER_1;
    ms_rel_resp[2] = OP_CH9329_ADDR_DEFAULT;
    ms_rel_resp[3] = OP_CH9329_RESP_MOUSE_REL;
    ms_rel_resp[4] = 0x05;
    memset(ms_rel_resp + 5, 0, 5);
    ms_rel_resp[10] = 0xFF;  /* bad checksum */
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR,
                  op_ch9329_parse_mouse_rel_response(ms_rel_resp, sizeof(ms_rel_resp), &ms_rel_parsed),
                  "mouse_rel bad checksum");

    /* Mouse abs response with bad checksum */
    ms_abs_resp[0] = OP_CH9329_HEADER_0;
    ms_abs_resp[1] = OP_CH9329_HEADER_1;
    ms_abs_resp[2] = OP_CH9329_ADDR_DEFAULT;
    ms_abs_resp[3] = OP_CH9329_RESP_MOUSE_ABS;
    ms_abs_resp[4] = 0x07;
    memset(ms_abs_resp + 5, 0, 7);
    ms_abs_resp[12] = 0xFF;  /* bad checksum */
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR,
                  op_ch9329_parse_mouse_abs_response(ms_abs_resp, sizeof(ms_abs_resp), &ms_abs_parsed),
                  "mouse_abs bad checksum");
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
    RUN_TEST(test_parse_keyboard_packet);
    RUN_TEST(test_parse_mouse_rel_packet);
    RUN_TEST(test_parse_mouse_abs_packet);
    RUN_TEST(test_parse_usb_switch_response_packet);
    RUN_TEST(test_parse_packet_null_raw);
    RUN_TEST(test_parse_packet_null_out);
    RUN_TEST(test_parse_packet_too_short);
    RUN_TEST(test_parse_packet_bad_header);
    RUN_TEST(test_parse_packet_bad_checksum);
    RUN_TEST(test_parse_packet_len_mismatch);
    RUN_TEST(test_parse_packet_roundtrip_all_types);
    RUN_TEST(test_parse_keyboard_response);
    RUN_TEST(test_parse_keyboard_response_null);
    RUN_TEST(test_parse_keyboard_response_bad_cmd);
    RUN_TEST(test_parse_mouse_rel_response);
    RUN_TEST(test_parse_mouse_rel_response_null);
    RUN_TEST(test_parse_mouse_abs_response);
    RUN_TEST(test_parse_mouse_abs_response_null);
    RUN_TEST(test_parse_response_bad_checksum);

    if (test_failures != 0) {
        fprintf(stderr, "protocol_ch9329_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("protocol_ch9329_test: all tests passed\n");
    return 0;
}
