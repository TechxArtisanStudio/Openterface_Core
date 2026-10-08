/**
 * @file serial_chip_test.c
 * @brief Tests for serial chip type identification and capability queries.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "openterface/serial_chip_types.h"

/* ── Test helpers ──────────────────────────────────────────────────── */

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do {                         \
    printf("  %-45s", #fn);                       \
    tests_run++;                                  \
    fn();                                         \
    tests_passed++;                               \
    printf("PASS\n");                             \
} while (0)

/* ── Detection tests ───────────────────────────────────────────────── */

static void test_detect_ch9329(void)
{
    op_serial_chip_type_t type = op_serial_chip_detect(0x1A86, 0x7523);
    assert(type == OP_SERIAL_CHIP_CH9329);
}

static void test_detect_ch32v208(void)
{
    op_serial_chip_type_t type = op_serial_chip_detect(0x1A86, 0xFE0C);
    assert(type == OP_SERIAL_CHIP_CH32V208);
}

static void test_detect_unknown(void)
{
    op_serial_chip_type_t type = op_serial_chip_detect(0x0000, 0x0000);
    assert(type == OP_SERIAL_CHIP_UNKNOWN);

    type = op_serial_chip_detect(0x1234, 0x5678);
    assert(type == OP_SERIAL_CHIP_UNKNOWN);
}

static void test_detect_wrong_pid(void)
{
    /* Right VID, wrong PID */
    op_serial_chip_type_t type = op_serial_chip_detect(0x1A86, 0x0000);
    assert(type == OP_SERIAL_CHIP_UNKNOWN);
}

/* ── Name tests ────────────────────────────────────────────────────── */

static void test_name_ch9329(void)
{
    const char *name = op_serial_chip_name(OP_SERIAL_CHIP_CH9329);
    assert(strcmp(name, "CH9329") == 0);
}

static void test_name_ch32v208(void)
{
    const char *name = op_serial_chip_name(OP_SERIAL_CHIP_CH32V208);
    assert(strcmp(name, "CH32V208") == 0);
}

static void test_name_unknown(void)
{
    const char *name = op_serial_chip_name(OP_SERIAL_CHIP_UNKNOWN);
    assert(strcmp(name, "Unknown") == 0);
}

/* ── Baudrate support tests ────────────────────────────────────────── */

static void test_ch9329_baudrate_support(void)
{
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH9329, 9600) == true);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH9329, 115200) == true);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH9329, 57600) == false);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH9329, 0) == false);
}

static void test_ch32v208_baudrate_support(void)
{
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH32V208, 115200) == true);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH32V208, 9600) == false);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_CH32V208, 57600) == false);
}

static void test_unknown_baudrate_support(void)
{
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_UNKNOWN, 9600) == false);
    assert(op_serial_chip_supports_baudrate(OP_SERIAL_CHIP_UNKNOWN, 115200) == false);
}

/* ── Default baudrate tests ────────────────────────────────────────── */

static void test_default_baudrate(void)
{
    assert(op_serial_chip_default_baudrate(OP_SERIAL_CHIP_CH9329) == 9600);
    assert(op_serial_chip_default_baudrate(OP_SERIAL_CHIP_CH32V208) == 115200);
    assert(op_serial_chip_default_baudrate(OP_SERIAL_CHIP_UNKNOWN) == 9600);
}

/* ── Capability tests ──────────────────────────────────────────────── */

static void test_command_config_support(void)
{
    assert(op_serial_chip_supports_command_config(OP_SERIAL_CHIP_CH9329) == true);
    assert(op_serial_chip_supports_command_config(OP_SERIAL_CHIP_CH32V208) == false);
}

static void test_usb_switch_support(void)
{
    assert(op_serial_chip_supports_usb_switch(OP_SERIAL_CHIP_CH9329) == false);
    assert(op_serial_chip_supports_usb_switch(OP_SERIAL_CHIP_CH32V208) == true);
}

/* ── Alternate baudrate tests ──────────────────────────────────────── */

static void test_alternate_baudrate_ch9329(void)
{
    assert(op_serial_chip_alternate_baudrate(OP_SERIAL_CHIP_CH9329, 9600) == 115200);
    assert(op_serial_chip_alternate_baudrate(OP_SERIAL_CHIP_CH9329, 115200) == 9600);
}

static void test_alternate_baudrate_ch32v208(void)
{
    /* CH32V208 has no alternate; returns same */
    assert(op_serial_chip_alternate_baudrate(OP_SERIAL_CHIP_CH32V208, 115200) == 115200);
}

/* ── VID/PID constant consistency tests ────────────────────────────── */

static void test_vid_pid_constants(void)
{
    /* Verify constants match the expected values */
    assert(OP_SERIAL_CHIP_CH9329_VID == 0x1A86u);
    assert(OP_SERIAL_CHIP_CH9329_PID == 0x7523u);
    assert(OP_SERIAL_CHIP_CH32V208_VID == 0x1A86u);
    assert(OP_SERIAL_CHIP_CH32V208_PID == 0xFE0Cu);
}

/* ── Main ──────────────────────────────────────────────────────────── */

int main(void)
{
    printf("serial_chip_test:\n");

    /* Detection */
    RUN_TEST(test_detect_ch9329);
    RUN_TEST(test_detect_ch32v208);
    RUN_TEST(test_detect_unknown);
    RUN_TEST(test_detect_wrong_pid);

    /* Name */
    RUN_TEST(test_name_ch9329);
    RUN_TEST(test_name_ch32v208);
    RUN_TEST(test_name_unknown);

    /* Baudrate support */
    RUN_TEST(test_ch9329_baudrate_support);
    RUN_TEST(test_ch32v208_baudrate_support);
    RUN_TEST(test_unknown_baudrate_support);

    /* Default baudrate */
    RUN_TEST(test_default_baudrate);

    /* Capability */
    RUN_TEST(test_command_config_support);
    RUN_TEST(test_usb_switch_support);

    /* Alternate baudrate */
    RUN_TEST(test_alternate_baudrate_ch9329);
    RUN_TEST(test_alternate_baudrate_ch32v208);

    /* Constants */
    RUN_TEST(test_vid_pid_constants);

    printf("Result: %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
