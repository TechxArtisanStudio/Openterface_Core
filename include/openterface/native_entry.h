#ifndef OPENTERFACE_CORE_NATIVE_H
#define OPENTERFACE_CORE_NATIVE_H

#include "openterface/chip.h"
#include "openterface/device.h"
#include "openterface/hid.h"
#include "openterface/native_common.h"
#include "openterface/usb_mode.h"
#include "openterface/video_status.h"

#ifdef __cplusplus
extern "C" {
#endif

op_version_t op_core_native_version(void);
const char *op_core_native_hid_backend_name(void);
const char *op_core_native_serial_backend_name(const op_device_info_t *device);
op_status_t op_native_transport_init_serial(op_transport_t *transport, const op_device_info_t *device);
void op_native_transport_release(op_transport_t *transport);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_CORE_NATIVE_H */
