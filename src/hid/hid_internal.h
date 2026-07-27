#ifndef OPENTERFACE_HID_INTERNAL_H
#define OPENTERFACE_HID_INTERNAL_H

#include "openterface/hid.h"
#include "platform_backend.h"

struct op_hid_device_session_t {
    op_device_info_t device;
    const op_platform_hid_backend_t *backend;
    void *backend_context;
    int is_open;
    int transaction_active;
};

struct op_hid_transaction_guard_t {
    op_hid_device_session_t *session;
    op_hid_transaction_options_t options;
    int entered;
};

#endif /* OPENTERFACE_HID_INTERNAL_H */
