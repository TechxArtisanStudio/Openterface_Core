#ifndef OPENTERFACE_VIDEO_CHIP_CONTROLLER_INTERNAL_H
#define OPENTERFACE_VIDEO_CHIP_CONTROLLER_INTERNAL_H

#include "openterface/chip.h"
#include "openterface/usb_mode.h"
#include "openterface/video_status.h"

struct op_video_chip_controller_t {
    op_hid_device_session_t *session;
    op_transport_t *transport;
    op_video_chip_kind_t chip_kind;
    op_usb_mode_t usb_mode;
    op_video_input_status_t cached_status;
    op_video_chip_register_set_t register_set;
};

#endif /* OPENTERFACE_VIDEO_CHIP_CONTROLLER_INTERNAL_H */
