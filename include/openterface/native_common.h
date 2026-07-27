#ifndef OPENTERFACE_NATIVE_COMMON_H
#define OPENTERFACE_NATIVE_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_STATUS_OK = 0,
    OP_STATUS_INVALID_ARGUMENT = 1,
    OP_STATUS_NOT_SUPPORTED = 2,
    OP_STATUS_NOT_FOUND = 3,
    OP_STATUS_IO_ERROR = 4,
    OP_STATUS_TIMEOUT = 5,
    OP_STATUS_BUSY = 6,
    OP_STATUS_UNINITIALIZED = 7
} op_status_t;

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} op_version_t;

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_NATIVE_COMMON_H */
