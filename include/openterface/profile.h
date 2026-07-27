#ifndef OPENTERFACE_PROFILE_H
#define OPENTERFACE_PROFILE_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/capability.h"
#include "openterface/device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OP_PROFILE_ID_CAP 64
#define OP_PROFILE_NAME_CAP 64

typedef enum {
    OP_DEVICE_FAMILY_UNKNOWN = 0,
    OP_DEVICE_FAMILY_MINI_KVM = 1,
    OP_DEVICE_FAMILY_KVM_GO = 2,
    OP_DEVICE_FAMILY_KEYMOD = 3
} op_device_family_t;

typedef enum {
    OP_PROTOCOL_NONE = 0,
    OP_PROTOCOL_CH9329 = 1u << 0,
    OP_PROTOCOL_CH32V208 = 1u << 1,
    OP_PROTOCOL_MS21XX_REGISTER = 1u << 2
} op_protocol_flags_t;

typedef struct {
    char profile_id[OP_PROFILE_ID_CAP];
    char display_name[OP_PROFILE_NAME_CAP];
    op_device_family_t family;
    uint16_t vendor_id;
    uint16_t product_id;
    uint32_t interface_flags;
    uint32_t protocol_flags;
    uint32_t default_baudrate;
    uint32_t chip_hint;
    op_capability_flags_t capabilities;
} op_device_profile_t;

const op_device_profile_t *op_profile_registry(void);
size_t op_profile_registry_count(void);
const op_device_profile_t *op_profile_find_by_id(const char *profile_id);
const op_device_profile_t *op_profile_match(uint16_t vendor_id, uint16_t product_id, uint32_t interface_flags);
const op_device_profile_t *op_profile_match_with_hint(uint16_t vendor_id, uint16_t product_id, uint32_t interface_flags, uint32_t chip_hint);
op_status_t op_profile_apply_to_device(const op_device_profile_t *profile, op_device_info_t *device);
int op_profile_has_capability(const op_device_profile_t *profile, op_capability_id_t capability);
const char *op_device_family_label(op_device_family_t family);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_PROFILE_H */
