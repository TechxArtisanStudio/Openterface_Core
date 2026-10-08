#ifndef TESTS_STUB_PLATFORM_STUB_H
#define TESTS_STUB_PLATFORM_STUB_H

#include "openterface/platform_backend.h"

/**
 * Get the stub HID backend for testing
 */
const op_platform_hid_backend_t *op_platform_get_stub_hid_backend(void);

/**
 * Get the stub serial backend for testing
 */
const op_platform_serial_backend_t *op_platform_get_stub_serial_backend(void);

#endif /* TESTS_STUB_PLATFORM_STUB_H */