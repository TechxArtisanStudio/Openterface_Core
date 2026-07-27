#include "openterface/version.h"

op_version_t op_core_version(void) {
    op_version_t version = {
        OPENTERFACE_CORE_VERSION_MAJOR,
        OPENTERFACE_CORE_VERSION_MINOR,
        OPENTERFACE_CORE_VERSION_PATCH
    };
    return version;
}

const char *op_core_version_string(void) {
    return "0.1.0";
}
