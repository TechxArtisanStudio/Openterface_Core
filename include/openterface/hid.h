#ifndef OPENTERFACE_HID_H
#define OPENTERFACE_HID_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/device.h"
#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct op_hid_device_session_t op_hid_device_session_t;
typedef struct op_hid_transaction_guard_t op_hid_transaction_guard_t;

typedef struct {
    uint32_t timeout_ms;
    uint32_t retry_count;
} op_hid_transaction_options_t;

op_status_t op_hid_device_session_create(const op_device_info_t *device, op_hid_device_session_t **out_session);
void op_hid_device_session_destroy(op_hid_device_session_t *session);
op_status_t op_hid_device_session_open(op_hid_device_session_t *session);
op_status_t op_hid_device_session_close(op_hid_device_session_t *session);
int op_hid_device_session_is_open(const op_hid_device_session_t *session);
const op_device_info_t *op_hid_device_session_device(const op_hid_device_session_t *session);
op_status_t op_hid_device_session_send_feature_report(op_hid_device_session_t *session, const uint8_t *report, size_t length);
op_status_t op_hid_device_session_get_feature_report(op_hid_device_session_t *session, uint8_t *report, size_t capacity, size_t *out_length);

op_status_t op_hid_transaction_guard_create(op_hid_device_session_t *session, const op_hid_transaction_options_t *options, op_hid_transaction_guard_t **out_guard);
void op_hid_transaction_guard_destroy(op_hid_transaction_guard_t *guard);
op_status_t op_hid_transaction_guard_enter(op_hid_transaction_guard_t *guard);
void op_hid_transaction_guard_leave(op_hid_transaction_guard_t *guard);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_HID_H */
