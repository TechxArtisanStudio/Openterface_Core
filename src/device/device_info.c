#include "openterface/device.h"

#include <string.h>

void op_device_info_init(op_device_info_t *device) {
    if (device == NULL) {
        return;
    }

    memset(device, 0, sizeof(*device));
}

int op_device_info_has_interface(const op_device_info_t *device, uint32_t interface_flag) {
    if (device == NULL) {
        return 0;
    }

    return (device->interface_flags & interface_flag) != 0u;
}

int op_device_info_has_capability(const op_device_info_t *device, op_capability_id_t capability) {
    if (device == NULL) {
        return 0;
    }

    return op_capabilities_has(device->capabilities, capability);
}
