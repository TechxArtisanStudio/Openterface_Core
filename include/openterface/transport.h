#ifndef OPENTERFACE_TRANSPORT_H
#define OPENTERFACE_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "openterface/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_TRANSPORT_KIND_UNKNOWN = 0,
    OP_TRANSPORT_KIND_SERIAL = 1,
    OP_TRANSPORT_KIND_HID_FEATURE = 2,
    OP_TRANSPORT_KIND_WEB_SERIAL = 3,
    OP_TRANSPORT_KIND_WEB_HID = 4,
    OP_TRANSPORT_KIND_LOCAL_SERVICE = 5,
    OP_TRANSPORT_KIND_BLE = 6,
    OP_TRANSPORT_KIND_STUB = 7
} op_transport_kind_t;

typedef struct op_transport_t op_transport_t;

typedef struct {
    op_status_t (*open)(void *context);
    op_status_t (*close)(void *context);
    op_status_t (*read)(void *context, uint8_t *buffer, size_t capacity, size_t *out_length);
    op_status_t (*write)(void *context, const uint8_t *buffer, size_t length);
    op_status_t (*control)(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length);
} op_transport_vtable_t;

struct op_transport_t {
    op_transport_kind_t kind;
    void *context;
    const op_transport_vtable_t *vtable;
    int is_open;
};

void op_transport_init(op_transport_t *transport, op_transport_kind_t kind, void *context, const op_transport_vtable_t *vtable);
op_status_t op_transport_open(op_transport_t *transport);
op_status_t op_transport_close(op_transport_t *transport);
op_status_t op_transport_read(op_transport_t *transport, uint8_t *buffer, size_t capacity, size_t *out_length);
op_status_t op_transport_write(op_transport_t *transport, const uint8_t *buffer, size_t length);
op_status_t op_transport_control(op_transport_t *transport, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length);
const char *op_transport_kind_label(op_transport_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_TRANSPORT_H */
