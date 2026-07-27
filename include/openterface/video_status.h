#ifndef OPENTERFACE_VIDEO_STATUS_H
#define OPENTERFACE_VIDEO_STATUS_H

#include <stdint.h>

#include "openterface/chip.h"
#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int hdmi_connected;
    uint16_t width;
    uint16_t height;
    uint32_t fps_milli;
    uint32_t pixel_clock_khz;
} op_video_input_status_t;

typedef struct op_video_status_poller_t op_video_status_poller_t;

op_status_t op_video_status_poller_create(op_video_chip_controller_t *controller, op_video_status_poller_t **out_poller);
void op_video_status_poller_destroy(op_video_status_poller_t *poller);
op_status_t op_video_status_poller_poll(op_video_status_poller_t *poller, op_video_input_status_t *out_status);
void op_video_input_status_clear(op_video_input_status_t *status);
void op_video_input_status_normalize(op_video_chip_kind_t chip_kind, op_video_input_status_t *status);
op_status_t op_video_chip_controller_get_cached_input_status(op_video_chip_controller_t *controller, op_video_input_status_t *out_status);
op_status_t op_video_chip_controller_set_cached_input_status(op_video_chip_controller_t *controller, const op_video_input_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_VIDEO_STATUS_H */
