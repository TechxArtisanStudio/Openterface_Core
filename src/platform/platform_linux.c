#include "platform_backend.h"

#if defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <linux/hidraw.h>

typedef struct {
    op_device_info_t device;
    int fd;
    int opened;
} op_platform_linux_context_t;

typedef struct {
    op_device_info_t device;
    int fd;
    int opened;
} op_platform_linux_serial_context_t;

static op_status_t op_platform_linux_map_errno(int error_code) {
    switch (error_code) {
        case ENOENT:
            return OP_STATUS_NOT_FOUND;
        case EBUSY:
            return OP_STATUS_BUSY;
        case ETIMEDOUT:
            return OP_STATUS_TIMEOUT;
        default:
            return OP_STATUS_IO_ERROR;
    }
}

static speed_t op_platform_linux_baudrate(const op_device_info_t *device) {
    uint32_t baudrate = device != NULL ? device->default_baudrate : 0u;

    switch (baudrate) {
        case 9600u:
            return B9600;
        case 19200u:
            return B19200;
        case 38400u:
            return B38400;
        case 57600u:
            return B57600;
        case 230400u:
            return B230400;
        case 115200u:
        default:
            return B115200;
    }
}

static void *op_platform_linux_create_context(const op_device_info_t *device) {
    op_platform_linux_context_t *context = (op_platform_linux_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    context->fd = -1;

    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
    }

    return context;
}

static void op_platform_linux_destroy_context(void *context) {
    op_platform_linux_context_t *state = (op_platform_linux_context_t *)context;

    if (state != NULL && state->fd >= 0) {
        close(state->fd);
        state->fd = -1;
    }

    free(context);
}

static void *op_platform_linux_serial_create_context(const op_device_info_t *device) {
    op_platform_linux_serial_context_t *context = (op_platform_linux_serial_context_t *)calloc(1, sizeof(*context));
    if (context == NULL) {
        return NULL;
    }

    context->fd = -1;
    if (device != NULL) {
        memcpy(&context->device, device, sizeof(*device));
    }

    return context;
}

static void op_platform_linux_serial_destroy_context(void *context) {
    op_platform_linux_serial_context_t *state = (op_platform_linux_serial_context_t *)context;

    if (state != NULL && state->fd >= 0) {
        close(state->fd);
        state->fd = -1;
    }

    free(context);
}

static op_status_t op_platform_linux_open(void *context) {
    op_platform_linux_context_t *state = (op_platform_linux_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (state->device.hid_path[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    if (state->fd >= 0 && !state->opened) {
        close(state->fd);
        state->fd = -1;
    }

    if (state->opened && state->fd >= 0) {
        return OP_STATUS_OK;
    }

    state->fd = open(state->device.hid_path, O_RDWR);
    if (state->fd < 0) {
        state->opened = 0;
        return op_platform_linux_map_errno(errno);
    }

    state->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_serial_open(void *context) {
    op_platform_linux_serial_context_t *state = (op_platform_linux_serial_context_t *)context;
    struct termios tty;
    speed_t baudrate;

    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (state->device.serial_path[0] == '\0') {
        return OP_STATUS_NOT_FOUND;
    }

    if (state->fd >= 0 && state->opened) {
        return OP_STATUS_OK;
    }

    if (state->fd >= 0) {
        close(state->fd);
        state->fd = -1;
    }

    state->fd = open(state->device.serial_path, O_RDWR | O_NOCTTY | O_SYNC);
    if (state->fd < 0) {
        state->opened = 0;
        return op_platform_linux_map_errno(errno);
    }

    if (tcgetattr(state->fd, &tty) != 0) {
        close(state->fd);
        state->fd = -1;
        return op_platform_linux_map_errno(errno);
    }

    cfmakeraw(&tty);
    baudrate = op_platform_linux_baudrate(&state->device);
    cfsetispeed(&tty, baudrate);
    cfsetospeed(&tty, baudrate);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(state->fd, TCSANOW, &tty) != 0) {
        close(state->fd);
        state->fd = -1;
        return op_platform_linux_map_errno(errno);
    }

    tcflush(state->fd, TCIOFLUSH);
    state->opened = 1;
    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_close(void *context) {
    op_platform_linux_context_t *state = (op_platform_linux_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (state->fd >= 0) {
        if (close(state->fd) != 0) {
            return op_platform_linux_map_errno(errno);
        }
        state->fd = -1;
    }

    state->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_serial_close(void *context) {
    op_platform_linux_serial_context_t *state = (op_platform_linux_serial_context_t *)context;
    if (state == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (state->fd >= 0) {
        if (close(state->fd) != 0) {
            return op_platform_linux_map_errno(errno);
        }
        state->fd = -1;
    }

    state->opened = 0;
    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_send_feature_report(void *context, const uint8_t *report, size_t length) {
    op_platform_linux_context_t *state = (op_platform_linux_context_t *)context;
    if (state == NULL || report == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (state->fd < 0) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (ioctl(state->fd, HIDIOCSFEATURE(length), (void *)report) < 0) {
        return op_platform_linux_map_errno(errno);
    }

    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_get_feature_report(void *context, uint8_t *report, size_t capacity, size_t *out_length) {
    op_platform_linux_context_t *state = (op_platform_linux_context_t *)context;
    if (state == NULL || report == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened) {
        return OP_STATUS_UNINITIALIZED;
    }

    if (state->fd < 0) {
        return OP_STATUS_UNINITIALIZED;
    }

    {
        int result = ioctl(state->fd, HIDIOCGFEATURE(capacity), report);
        if (result < 0) {
            if (out_length != NULL) {
                *out_length = 0u;
            }
            return op_platform_linux_map_errno(errno);
        }

        if (out_length != NULL) {
            *out_length = (size_t)result;
        }
    }

    return OP_STATUS_OK;
}

static op_status_t op_platform_linux_serial_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    op_platform_linux_serial_context_t *state = (op_platform_linux_serial_context_t *)context;
    fd_set read_set;
    struct timeval timeout;
    ssize_t result;

    if (state == NULL || buffer == NULL || capacity == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened || state->fd < 0) {
        return OP_STATUS_UNINITIALIZED;
    }

    FD_ZERO(&read_set);
    FD_SET(state->fd, &read_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    result = select(state->fd + 1, &read_set, NULL, NULL, &timeout);
    if (result < 0) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return op_platform_linux_map_errno(errno);
    }
    if (result == 0) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return OP_STATUS_TIMEOUT;
    }

    result = read(state->fd, buffer, capacity);
    if (result < 0) {
        if (out_length != NULL) {
            *out_length = 0u;
        }
        return op_platform_linux_map_errno(errno);
    }

    if (out_length != NULL) {
        *out_length = (size_t)result;
    }

    return result == 0 ? OP_STATUS_TIMEOUT : OP_STATUS_OK;
}

static op_status_t op_platform_linux_serial_write(void *context, const uint8_t *buffer, size_t length) {
    op_platform_linux_serial_context_t *state = (op_platform_linux_serial_context_t *)context;
    ssize_t result;

    if (state == NULL || buffer == NULL || length == 0u) {
        return OP_STATUS_INVALID_ARGUMENT;
    }
    if (!state->opened || state->fd < 0) {
        return OP_STATUS_UNINITIALIZED;
    }

    result = write(state->fd, buffer, length);
    if (result < 0) {
        return op_platform_linux_map_errno(errno);
    }

    if (tcdrain(state->fd) != 0) {
        return op_platform_linux_map_errno(errno);
    }

    return (size_t)result == length ? OP_STATUS_OK : OP_STATUS_IO_ERROR;
}

static const op_platform_hid_backend_t OP_PLATFORM_LINUX_BACKEND = {
    op_platform_linux_create_context,
    op_platform_linux_destroy_context,
    op_platform_linux_open,
    op_platform_linux_close,
    op_platform_linux_send_feature_report,
    op_platform_linux_get_feature_report,
};

static const op_platform_serial_backend_t OP_PLATFORM_LINUX_SERIAL_BACKEND = {
    op_platform_linux_serial_create_context,
    op_platform_linux_serial_destroy_context,
    op_platform_linux_serial_open,
    op_platform_linux_serial_close,
    op_platform_linux_serial_read,
    op_platform_linux_serial_write,
};

const op_platform_hid_backend_t *op_platform_get_linux_hid_backend(void) {
    return &OP_PLATFORM_LINUX_BACKEND;
}

const op_platform_serial_backend_t *op_platform_get_linux_serial_backend(void) {
    return &OP_PLATFORM_LINUX_SERIAL_BACKEND;
}

#else

const op_platform_hid_backend_t *op_platform_get_linux_hid_backend(void) {
    return op_platform_get_stub_hid_backend();
}

const op_platform_serial_backend_t *op_platform_get_linux_serial_backend(void) {
    return op_platform_get_stub_serial_backend();
}

#endif
