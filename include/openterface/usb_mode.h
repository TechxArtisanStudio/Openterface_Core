#ifndef OPENTERFACE_USB_MODE_H
#define OPENTERFACE_USB_MODE_H

#include "openterface/chip.h"
#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_USB_MODE_UNKNOWN = 0,
    OP_USB_MODE_HOST = 1,
    OP_USB_MODE_TARGET = 2
} op_usb_mode_t;

typedef struct {
    const op_device_info_t *device;
    op_hid_device_session_t *hid_session;
    op_transport_t *transport;
    op_video_chip_controller_t *chip_controller;
    op_usb_mode_t cached_mode;
} op_usb_mode_endpoint_t;

void op_usb_mode_endpoint_init(op_usb_mode_endpoint_t *endpoint);
op_status_t op_usb_mode_endpoint_bind_device(op_usb_mode_endpoint_t *endpoint, const op_device_info_t *device);
op_status_t op_usb_mode_endpoint_bind_hid_session(op_usb_mode_endpoint_t *endpoint, op_hid_device_session_t *session);
op_status_t op_usb_mode_endpoint_bind_transport(op_usb_mode_endpoint_t *endpoint, op_transport_t *transport);
op_status_t op_usb_mode_endpoint_bind_chip_controller(op_usb_mode_endpoint_t *endpoint, op_video_chip_controller_t *controller);

op_status_t op_usb_mode_get(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t *out_mode);
op_status_t op_usb_mode_set(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t mode);

op_status_t op_video_chip_controller_get_usb_mode(op_video_chip_controller_t *controller, op_usb_mode_t *out_mode);
op_status_t op_video_chip_controller_set_usb_mode(op_video_chip_controller_t *controller, op_usb_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_USB_MODE_H */
