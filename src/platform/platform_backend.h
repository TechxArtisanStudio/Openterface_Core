#ifndef OPENTERFACE_PLATFORM_BACKEND_H
#define OPENTERFACE_PLATFORM_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/device.h"
#include "openterface/native_common.h"

typedef struct {
    void *(*create_context)(const op_device_info_t *device);
    void (*destroy_context)(void *context);
    op_status_t (*open)(void *context);
    op_status_t (*close)(void *context);
    op_status_t (*send_feature_report)(void *context, const uint8_t *report, size_t length);
    op_status_t (*get_feature_report)(void *context, uint8_t *report, size_t capacity, size_t *out_length);
} op_platform_hid_backend_t;

typedef struct {
    void *(*create_context)(const op_device_info_t *device);
    void (*destroy_context)(void *context);
    op_status_t (*open)(void *context);
    op_status_t (*close)(void *context);
    op_status_t (*read)(void *context, uint8_t *buffer, size_t capacity, size_t *out_length);
    op_status_t (*write)(void *context, const uint8_t *buffer, size_t length);
} op_platform_serial_backend_t;

const op_platform_hid_backend_t *op_platform_get_hid_backend(void);
const char *op_platform_get_hid_backend_name(void);

const op_platform_hid_backend_t *op_platform_get_windows_hid_backend(void);
const op_platform_hid_backend_t *op_platform_get_linux_hid_backend(void);
const op_platform_hid_backend_t *op_platform_get_stub_hid_backend(void);

const op_platform_serial_backend_t *op_platform_get_serial_backend(void);
const op_platform_serial_backend_t *op_platform_get_serial_backend_for_device(const op_device_info_t *device);
const char *op_platform_get_serial_backend_name(void);
const char *op_platform_get_serial_backend_name_for_device(const op_device_info_t *device);

const op_platform_serial_backend_t *op_platform_get_windows_serial_backend(void);
const op_platform_serial_backend_t *op_platform_get_linux_serial_backend(void);
const op_platform_serial_backend_t *op_platform_get_stub_serial_backend(void);

#endif /* OPENTERFACE_PLATFORM_BACKEND_H */
