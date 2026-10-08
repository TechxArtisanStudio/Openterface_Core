#ifndef OPENTERFACE_CORE_NATIVE_H
#define OPENTERFACE_CORE_NATIVE_H

#include "openterface/chip.h"
#include "openterface/device.h"
#include "openterface/hid.h"
#include "openterface/native_common.h"
#include "openterface/platform_backend.h"
#include "openterface/usb_mode.h"
#include "openterface/video_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register platform backends
 *
 * Frontends call this at initialization to provide their platform-specific
 * backend implementations. After registration, Core uses the registered
 * backends for all device communication.
 *
 * @param backends Platform backends collection
 * @return OP_STATUS_OK on success
 *
 * @note Must be called before any other Core API
 * @note The backends pointer must remain valid for the application lifetime
 */
op_status_t op_core_register_platform_backends(const op_platform_backends_t *backends);

/**
 * @brief Get the registered HID backend
 *
 * @return HID backend pointer, NULL if not registered
 */
const op_platform_hid_backend_t *op_core_get_hid_backend(void);

/**
 * @brief Get the registered serial backend
 *
 * @return Serial backend pointer, NULL if not registered
 */
const op_platform_serial_backend_t *op_core_get_serial_backend(void);

op_version_t op_core_native_version(void);
const char *op_core_native_hid_backend_name(void);
const char *op_core_native_serial_backend_name(const op_device_info_t *device);
op_status_t op_native_transport_init_serial(op_transport_t *transport, const op_device_info_t *device);
void op_native_transport_release(op_transport_t *transport);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_CORE_NATIVE_H */
