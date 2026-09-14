/*
 * test_helpers.h - Shared test utilities for Openterface Core test suite
 *
 * Provides unified assertion macros and test runner macros.
 * Each test file must define: int test_failures = 0;
 *
 * Usage:
 *   #include "test_helpers.h"
 *   int test_failures = 0;
 *
 *   static void test_example(void) {
 *       ASSERT_EQ_INT(42, some_function(), "example returns 42");
 *       ASSERT_EQ_STR("hello", get_string(), "string matches");
 *       ASSERT_TRUE(ptr != NULL, "pointer is valid");
 *   }
 *
 *   int main(void) {
 *       printf("Running example tests...\n");
 *       RUN_TEST(test_example);
 *       if (test_failures != 0) {
 *           printf("example_test: %d failure(s)\n", test_failures);
 *           return 1;
 *       }
 *       printf("example_test: all tests passed\n");
 *       return 0;
 *   }
 */

#ifndef OP_TEST_HELPERS_H
#define OP_TEST_HELPERS_H

#include <stdio.h>
#include <string.h>

/* Global failure counter - each test file must define this */
extern int test_failures;

/* Integer equality check with decimal and hex output */
#define ASSERT_EQ_INT(expected, actual, msg) do { \
    if ((int)(expected) != (int)(actual)) { \
        fprintf(stderr, "  FAIL: %s: expected %d (0x%02X), got %d (0x%02X)\n", \
                msg, (int)(expected), (int)(expected), \
                (int)(actual), (int)(actual)); \
        test_failures++; \
    } \
} while (0)

/* String equality check */
#define ASSERT_EQ_STR(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, "  FAIL: %s: expected '%s', got '%s'\n", \
                msg, (expected), (actual)); \
        test_failures++; \
    } \
} while (0)

/* Boolean/pointer truth check */
#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s\n", msg); \
        test_failures++; \
    } \
} while (0)

/* Not-equal check (fails when actual equals the bad value) */
#define ASSERT_NEQ(bad, actual, msg) do { \
    if ((int)(bad) == (int)(actual)) { \
        fprintf(stderr, "  FAIL: %s: expected != %d (0x%02X)\n", \
                msg, (int)(bad), (int)(bad)); \
        test_failures++; \
    } \
} while (0)

/* Run a single test function and report pass/fail */
#define RUN_TEST(fn) do { \
    int _before = test_failures; \
    printf("  Running %s...\n", #fn); \
    fn(); \
    if (test_failures == _before) \
        printf("  [PASS] %s\n", #fn); \
    else \
        printf("  [FAIL] %s\n", #fn); \
} while (0)

#endif /* OP_TEST_HELPERS_H */
