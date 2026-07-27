#ifndef OPENTERFACE_CHIP_H
#define OPENTERFACE_CHIP_H

#include <stdint.h>

#include "openterface/hid.h"
#include "openterface/native_common.h"
#include "openterface/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_VIDEO_CHIP_UNKNOWN = 0,
    OP_VIDEO_CHIP_MS2109 = 1,
    OP_VIDEO_CHIP_MS2109S = 2,
    OP_VIDEO_CHIP_MS2130S = 3
} op_video_chip_kind_t;

typedef struct {
    uint16_t hdmi_status;
    uint16_t gpio0;
    uint16_t spdifout;
    uint16_t firmware_version[4];
    uint16_t width_h;
    uint16_t width_l;
    uint16_t height_h;
    uint16_t height_l;
    uint16_t fps_h;
    uint16_t fps_l;
    uint16_t pixel_clock_h;
    uint16_t pixel_clock_l;
} op_video_chip_register_set_t;

typedef struct op_video_chip_controller_t op_video_chip_controller_t;

op_video_chip_kind_t op_video_chip_detect_from_device(const op_device_info_t *device);
const char *op_video_chip_kind_label(op_video_chip_kind_t kind);
int op_video_chip_get_register_set(op_video_chip_kind_t kind, op_video_chip_register_set_t *out_registers);

op_status_t op_video_chip_controller_create(op_hid_device_session_t *session, op_video_chip_controller_t **out_controller);
void op_video_chip_controller_destroy(op_video_chip_controller_t *controller);
op_status_t op_video_chip_controller_set_transport(op_video_chip_controller_t *controller, op_transport_t *transport);
op_status_t op_video_chip_controller_detect(op_video_chip_controller_t *controller, op_video_chip_kind_t *out_kind);
op_status_t op_video_chip_controller_get_register_set(op_video_chip_controller_t *controller, op_video_chip_register_set_t *out_registers);
op_status_t op_video_chip_controller_read_register(op_video_chip_controller_t *controller, uint16_t address, uint8_t *out_value);
op_status_t op_video_chip_controller_write_register(op_video_chip_controller_t *controller, uint16_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_CHIP_H */
