#ifndef OPENTERFACE_PLATFORM_BACKEND_H
#define OPENTERFACE_PLATFORM_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/device.h"
#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HID backend interface
 *
 * Frontends implement these functions and register with Core.
 * Core uses the interface only, with no platform-specific implementation.
 */
typedef struct {
    /** Create backend context. Returns NULL on failure. */
    void *(*create_context)(const op_device_info_t *device);

    /** Destroy backend context. */
    void (*destroy_context)(void *context);

    /** Open the device. Returns OP_STATUS_OK on success. */
    op_status_t (*open)(void *context);

    /** Close the device. Returns OP_STATUS_OK on success. */
    op_status_t (*close)(void *context);

    /** Send an HID feature report. */
    op_status_t (*send_feature_report)(void *context, const uint8_t *report, size_t length);

    /** Receive an HID feature report. */
    op_status_t (*get_feature_report)(void *context, uint8_t *report, size_t capacity, size_t *out_length);
} op_platform_hid_backend_t;

/**
 * @brief Serial backend interface
 *
 * Frontends implement these functions and register with Core.
 */
typedef struct {
    /** Create backend context. Returns NULL on failure. */
    void *(*create_context)(const op_device_info_t *device);

    /** Destroy backend context. */
    void (*destroy_context)(void *context);

    /** Open the serial port. Returns OP_STATUS_OK on success. */
    op_status_t (*open)(void *context);

    /** Close the serial port. Returns OP_STATUS_OK on success. */
    op_status_t (*close)(void *context);

    /** Read data from serial port. */
    op_status_t (*read)(void *context, uint8_t *buffer, size_t capacity, size_t *out_length);

    /** Write data to serial port. */
    op_status_t (*write)(void *context, const uint8_t *buffer, size_t length);
} op_platform_serial_backend_t;

/**
 * @brief Platform backends collection
 *
 * Frontends create this struct at initialization and register it with Core.
 */
typedef struct {
    const op_platform_hid_backend_t *hid_backend;
    const op_platform_serial_backend_t *serial_backend;
} op_platform_backends_t;

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_PLATFORM_BACKEND_H */
