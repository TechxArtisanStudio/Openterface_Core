#include "platform_backend.h"

#include <string.h>

static int op_platform_device_uses_stub_serial(const op_device_info_t *device) {
    if (device == NULL) {
        return 0;
    }

    return strncmp(device->serial_path, "stub:", 5u) == 0;
}

const op_platform_hid_backend_t *op_platform_get_hid_backend(void) {
#if defined(_WIN32)
    return op_platform_get_windows_hid_backend();
#elif defined(__linux__)
    return op_platform_get_linux_hid_backend();
#else
    return op_platform_get_stub_hid_backend();
#endif
}

const char *op_platform_get_hid_backend_name(void) {
#if defined(_WIN32)
    return "windows";
#elif defined(__linux__)
    return "linux";
#else
    return "stub";
#endif
}

const op_platform_serial_backend_t *op_platform_get_serial_backend(void) {
#if defined(_WIN32)
    return op_platform_get_windows_serial_backend();
#elif defined(__linux__)
    return op_platform_get_linux_serial_backend();
#else
    return op_platform_get_stub_serial_backend();
#endif
}

const op_platform_serial_backend_t *op_platform_get_serial_backend_for_device(const op_device_info_t *device) {
    if (op_platform_device_uses_stub_serial(device)) {
        return op_platform_get_stub_serial_backend();
    }

    return op_platform_get_serial_backend();
}

const char *op_platform_get_serial_backend_name(void) {
#if defined(_WIN32)
    return "windows";
#elif defined(__linux__)
    return "linux";
#else
    return "stub";
#endif
}

const char *op_platform_get_serial_backend_name_for_device(const op_device_info_t *device) {
    if (op_platform_device_uses_stub_serial(device)) {
        return "stub";
    }

    return op_platform_get_serial_backend_name();
}
