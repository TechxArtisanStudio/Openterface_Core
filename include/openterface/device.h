#ifndef OPENTERFACE_DEVICE_H
#define OPENTERFACE_DEVICE_H

#include <stdint.h>

#include "openterface/capability.h"
#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OP_DEVICE_ID_CAP 64
#define OP_PORT_CHAIN_CAP 128
#define OP_DEVICE_PATH_CAP 512

typedef enum {
    OP_DEVICE_IF_NONE = 0,
    OP_DEVICE_IF_HID = 1u << 0,
    OP_DEVICE_IF_SERIAL = 1u << 1,
    OP_DEVICE_IF_CAMERA = 1u << 2,
    OP_DEVICE_IF_AUDIO = 1u << 3
} op_device_interface_flags_t;

typedef struct {
    char device_id[OP_DEVICE_ID_CAP];
    char profile_id[OP_DEVICE_ID_CAP];
    char port_chain[OP_PORT_CHAIN_CAP];
    char hid_path[OP_DEVICE_PATH_CAP];
    char serial_path[OP_DEVICE_PATH_CAP];
    char camera_path[OP_DEVICE_PATH_CAP];
    char audio_path[OP_DEVICE_PATH_CAP];
    uint16_t vendor_id;
    uint16_t product_id;
    uint32_t interface_flags;
    uint32_t protocol_flags;
    uint32_t chip_hint;
    uint32_t default_baudrate;
    op_capability_flags_t capabilities;
} op_device_info_t;

void op_device_info_init(op_device_info_t *device);
int op_device_info_has_interface(const op_device_info_t *device, uint32_t interface_flag);
int op_device_info_has_capability(const op_device_info_t *device, op_capability_id_t capability);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_DEVICE_H */
