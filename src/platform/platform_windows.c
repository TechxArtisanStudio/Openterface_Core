#include "platform_backend.h"

#if defined(_WIN32)

#include <windows.h>

#pragma comment(lib, "hid.lib")

#include <hidsdi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    op_device_info_t device;
    HANDLE handle;
    int opened;
} op_platform_windows_context_t;

typedef struct {
    op_device_info_t device;
    HANDLE handle;
    int opened;
} op_platform_windows_serial_context_t;

static op_status_t op_platform_windows_map_error(DWORD error_code) {
    switch (error_code) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return OP_STATUS_NOT_FOUND;
        case ERROR_BUSY:
        case ERROR_SHARING_VIOLATION:
            return OP_STATUS_BUSY;
        case WAIT_TIMEOUT:
        case ERROR_SEM_TIMEOUT:
            return OP_STATUS_TIMEOUT;
        default:
            return OP_STATUS_IO_ERROR;
    }
}

static DWORD op_platform_windows_serial_baudrate(const op_device_info_t *device) {
    if (device == NULL || device->default_baudrate == 0u) {
        return CBR_115200;
    }

    return (DWORD)device->default_baudrate;
}

static void op_platform_windows_serial_path(const op_device_info_t *device, char *buffer, size_t capacity) {
    const char *path;

    if (buffer == NULL || capacity == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (device == NULL) {
        return;
    }

    path = device->serial_path;
    if (path[0] == '\0') {
        return;
    }

    if (strncmp(path, "\\\\.\\", 4u) == 0) {
        strncpy(buffer, path, capacity - 1u);
        buffer[capacity - 1u] = '\0';
        return;
    }

    if ((path[0] == 'C' || path[0] == 'c') && (path[1] == 'O' || path[1] == 'o') && (path[2] == 'M' || path[2] == 'm')) {
        _snprintf(buffer, capacity, "\\\\.\\%s", path);
        buffer[capacity - 1u] = '\0';
        return;
    }

    strncpy(buffer, path, capacity - 1u);
    buffer[capacity - 1u] = '\0';
}

static void *op_platform_windows_create_context(const op_device_info_t *device) {
    op_platform_windows_context_t *context = (op_platform_windows_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    context->handle = INVALID_HANDLE_VALUE;

    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
    }

    return context;
}

static void op_platform_windows_destroy_context(void *context) {
    op_platform_windows_context_t *state = (op_platform_windows_context_t *)context;

    if (state != NULL && state->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    free(context);
}

static void *op_platform_windows_serial_create_context(const op_device_info_t *device) {
    op_platform_windows_serial_context_t *context = (op_platform_windows_serial_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    context->handle = INVALID_HANDLE_VALUE;
    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
    }

    return context;
}

static void op_platform_windows_serial_destroy_context(void *context) {
    op_platform_windows_serial_context_t *state = (op_platform_windows_serial_context_t *)context;

    if (state != NULL && state->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    free(context);
}

static op_status_t op_platform_windows_open(void *context) {
    op_platform_windows_context_t *state = (op_platform_windows_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (state->device.hid_path[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    if (state->handle != INVALID_HANDLE_VALUE && !state->opened) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    if (state->opened && state->handle != INVALID_HANDLE_VALUE) {
        return OP_STATUS_OK;
    }

    state->handle = CreateFileA(
        state->device.hid_path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (state->handle == INVALID_HANDLE_VALUE) {
        state->opened = 0;
        return op_platform_windows_map_error(GetLastError());
    }

    state->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_serial_open(void *context) {
    op_platform_windows_serial_context_t *state = (op_platform_windows_serial_context_t *)context;
    DCB dcb;
    COMMTIMEOUTS timeouts;
    char path_buffer[OP_DEVICE_PATH_CAP + 8u];

    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    op_platform_windows_serial_path(&state->device, path_buffer, sizeof(path_buffer));
    if (path_buffer[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    if (state->handle != INVALID_HANDLE_VALUE && state->opened) {
        return OP_STATUS_OK;
    }

    if (state->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    state->handle = CreateFileA(
        path_buffer,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (state->handle == INVALID_HANDLE_VALUE) {
        state->opened = 0;
        return op_platform_windows_map_error(GetLastError());
    }

    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(state->handle, &dcb)) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
        return op_platform_windows_map_error(GetLastError());
    }

    dcb.BaudRate = op_platform_windows_serial_baudrate(&state->device);
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(state->handle, &dcb)) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
        return op_platform_windows_map_error(GetLastError());
    }

    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = 50u;
    timeouts.ReadTotalTimeoutConstant = 100u;
    timeouts.ReadTotalTimeoutMultiplier = 10u;
    timeouts.WriteTotalTimeoutConstant = 100u;
    timeouts.WriteTotalTimeoutMultiplier = 10u;
    if (!SetCommTimeouts(state->handle, &timeouts)) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
        return op_platform_windows_map_error(GetLastError());
    }

    PurgeComm(state->handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    state->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_close(void *context) {
    op_platform_windows_context_t *state = (op_platform_windows_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (state->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    state->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_serial_close(void *context) {
    op_platform_windows_serial_context_t *state = (op_platform_windows_serial_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (state->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->handle);
        state->handle = INVALID_HANDLE_VALUE;
    }

    state->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_send_feature_report(void *context, const uint8_t *report, size_t length) {
    op_platform_windows_context_t *state = (op_platform_windows_context_t *)context;
    if (state == NULL || report == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (state->handle == INVALID_HANDLE_VALUE) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (!HidD_SetFeature(state->handle, (PVOID)report, (ULONG)length)) {
        return op_platform_windows_map_error(GetLastError());
    }

    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_get_feature_report(void *context, uint8_t *report, size_t capacity, size_t *out_length) {
    op_platform_windows_context_t *state = (op_platform_windows_context_t *)context;
    if (state == NULL || report == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (state->handle == INVALID_HANDLE_VALUE) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (!HidD_GetFeature(state->handle, report, (ULONG)capacity)) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return op_platform_windows_map_error(GetLastError());
    }

    if (out_length != NULL) {
        *out_length = capacity;
    }

    return OP_STATUS_OK;
}

static op_status_t op_platform_windows_serial_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    op_platform_windows_serial_context_t *state = (op_platform_windows_serial_context_t *)context;
    DWORD bytes_read = 0u;

    if (state == NULL || buffer == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!state->opened || state->handle == INVALID_HANDLE_VALUE) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (!ReadFile(state->handle, buffer, (DWORD)capacity, &bytes_read, NULL)) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return op_platform_windows_map_error(GetLastError());
    }

    if (out_length != NULL) {
        *out_length = (size_t)bytes_read;
    }

    return bytes_read == 0u ? OP_STATUS_TIMEOUT : OP_STATUS_OK;
}

static op_status_t op_platform_windows_serial_write(void *context, const uint8_t *buffer, size_t length) {
    op_platform_windows_serial_context_t *state = (op_platform_windows_serial_context_t *)context;
    DWORD bytes_written = 0u;

    if (state == NULL || buffer == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (!state->opened || state->handle == INVALID_HANDLE_VALUE) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (!WriteFile(state->handle, buffer, (DWORD)length, &bytes_written, NULL)) {
        return op_platform_windows_map_error(GetLastError());
    }

    return bytes_written == (DWORD)length ? OP_STATUS_OK : OP_STATUS_IO_ERROR;
}

static const op_platform_hid_backend_t OP_PLATFORM_WINDOWS_BACKEND = {
    op_platform_windows_create_context,
    op_platform_windows_destroy_context,
    op_platform_windows_open,
    op_platform_windows_close,
    op_platform_windows_send_feature_report,
    op_platform_windows_get_feature_report,
};

static const op_platform_serial_backend_t OP_PLATFORM_WINDOWS_SERIAL_BACKEND = {
    op_platform_windows_serial_create_context,
    op_platform_windows_serial_destroy_context,
    op_platform_windows_serial_open,
    op_platform_windows_serial_close,
    op_platform_windows_serial_read,
    op_platform_windows_serial_write,
};

const op_platform_hid_backend_t *op_platform_get_windows_hid_backend(void) {
    return &OP_PLATFORM_WINDOWS_BACKEND;
}

const op_platform_serial_backend_t *op_platform_get_windows_serial_backend(void) {
    return &OP_PLATFORM_WINDOWS_SERIAL_BACKEND;
}

#else

const op_platform_hid_backend_t *op_platform_get_windows_hid_backend(void) {
    return op_platform_get_stub_hid_backend();
}

const op_platform_serial_backend_t *op_platform_get_windows_serial_backend(void) {
    return op_platform_get_stub_serial_backend();
}

#endif
