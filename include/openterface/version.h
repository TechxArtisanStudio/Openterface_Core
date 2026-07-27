#ifndef OPENTERFACE_VERSION_H
#define OPENTERFACE_VERSION_H

#include "openterface/native_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPENTERFACE_CORE_VERSION_MAJOR 0
#define OPENTERFACE_CORE_VERSION_MINOR 1
#define OPENTERFACE_CORE_VERSION_PATCH 0

op_version_t op_core_version(void);
const char *op_core_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENTERFACE_VERSION_H */
