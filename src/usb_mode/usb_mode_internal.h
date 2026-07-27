#ifndef OPENTERFACE_USB_MODE_INTERNAL_H
#define OPENTERFACE_USB_MODE_INTERNAL_H

#include "video_chip_controller_internal.h"

typedef struct {
    int (*matches)(const op_usb_mode_endpoint_t *endpoint);
    op_status_t (*get_mode)(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t *out_mode);
    op_status_t (*set_mode)(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t mode);
} op_usb_mode_provider_t;

const op_usb_mode_provider_t *op_usb_mode_ms21xx_provider(void);
const op_usb_mode_provider_t *op_usb_mode_serial_provider(void);

#endif /* OPENTERFACE_USB_MODE_INTERNAL_H */