#include "usb_mode_internal.h"

typedef const op_usb_mode_provider_t *(*op_usb_mode_provider_factory_t)(void);

static const op_device_info_t *op_usb_mode_endpoint_device(const op_usb_mode_endpoint_t *endpoint) {
    if (endpoint == NULL) {
        return NULL;
    }

    if (endpoint->device != NULL) {
        return endpoint->device;
    }

    if (endpoint->hid_session != NULL) {
        return op_hid_device_session_device(endpoint->hid_session);
    }

    if (endpoint->chip_controller != NULL && endpoint->chip_controller->session != NULL) {
        return op_hid_device_session_device(endpoint->chip_controller->session);
    }

    return NULL;
}

static void op_usb_mode_endpoint_sync_to_controller(const op_usb_mode_endpoint_t *endpoint) {
    if (endpoint == NULL || endpoint->chip_controller == NULL) {
        return;
    }

    endpoint->chip_controller->transport = endpoint->transport;
    endpoint->chip_controller->usb_mode = endpoint->cached_mode;
}

static const op_usb_mode_provider_t *op_usb_mode_select_provider(const op_usb_mode_endpoint_t *endpoint) {
    static const op_usb_mode_provider_factory_t provider_factories[] = {
        op_usb_mode_ms21xx_provider,
        op_usb_mode_serial_provider
    };
    size_t index;
    const op_usb_mode_provider_t *provider;

    if (endpoint == NULL) {
        return NULL;
    }

    for (index = 0u; index < sizeof(provider_factories) / sizeof(provider_factories[0]); ++index) {
        provider = provider_factories[index]();
        if (provider != NULL && provider->matches != NULL && provider->matches(endpoint)) {
            return provider;
        }
    }

    return NULL;
}

static op_status_t op_usb_mode_validate_capability(const op_usb_mode_endpoint_t *endpoint) {
    const op_device_info_t *device = op_usb_mode_endpoint_device(endpoint);

    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (device == NULL) {
        return OP_STATUS_NOT_FOUND;
    }

    if (device->capabilities != 0u && !op_device_info_has_capability(device, OP_CAPABILITY_USB_ROLE_SWITCH)) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    return OP_STATUS_OK;
}

void op_usb_mode_endpoint_init(op_usb_mode_endpoint_t *endpoint) {
    if (endpoint == NULL) {
        return;
    }

    endpoint->device = NULL;
    endpoint->hid_session = NULL;
    endpoint->transport = NULL;
    endpoint->chip_controller = NULL;
    endpoint->cached_mode = OP_USB_MODE_UNKNOWN;
}

op_status_t op_usb_mode_endpoint_bind_device(op_usb_mode_endpoint_t *endpoint, const op_device_info_t *device) {
    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    endpoint->device = device;
    return OP_STATUS_OK;
}

op_status_t op_usb_mode_endpoint_bind_hid_session(op_usb_mode_endpoint_t *endpoint, op_hid_device_session_t *session) {
    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    endpoint->hid_session = session;
    if (session != NULL) {
        endpoint->device = op_hid_device_session_device(session);
    }
    return OP_STATUS_OK;
}

op_status_t op_usb_mode_endpoint_bind_transport(op_usb_mode_endpoint_t *endpoint, op_transport_t *transport) {
    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    endpoint->transport = transport;
    op_usb_mode_endpoint_sync_to_controller(endpoint);
    return OP_STATUS_OK;
}

op_status_t op_usb_mode_endpoint_bind_chip_controller(op_usb_mode_endpoint_t *endpoint, op_video_chip_controller_t *controller) {
    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    endpoint->chip_controller = controller;
    if (controller != NULL) {
        endpoint->hid_session = controller->session;
        endpoint->transport = controller->transport;
        endpoint->cached_mode = controller->usb_mode;
        endpoint->device = controller->session != NULL ? op_hid_device_session_device(controller->session) : endpoint->device;
    }
    return OP_STATUS_OK;
}

op_status_t op_usb_mode_get(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t *out_mode) {
    const op_usb_mode_provider_t *provider;
    op_status_t status;

    if (endpoint == NULL || out_mode == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_usb_mode_validate_capability(endpoint);
    if (status != OP_STATUS_OK) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return status;
    }

    provider = op_usb_mode_select_provider(endpoint);
    if (provider == NULL || provider->get_mode == NULL) {
        *out_mode = OP_USB_MODE_UNKNOWN;
        return OP_STATUS_NOT_SUPPORTED;
    }

    status = provider->get_mode(endpoint, out_mode);
    if (status == OP_STATUS_OK) {
        endpoint->cached_mode = *out_mode;
        op_usb_mode_endpoint_sync_to_controller(endpoint);
        return OP_STATUS_OK;
    }

    if (endpoint->cached_mode != OP_USB_MODE_UNKNOWN) {
        *out_mode = endpoint->cached_mode;
        return OP_STATUS_OK;
    }

    *out_mode = OP_USB_MODE_UNKNOWN;
    return status;
}

op_status_t op_usb_mode_set(op_usb_mode_endpoint_t *endpoint, op_usb_mode_t mode) {
    const op_usb_mode_provider_t *provider;
    op_status_t status;

    if (endpoint == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    if (mode != OP_USB_MODE_HOST && mode != OP_USB_MODE_TARGET) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    status = op_usb_mode_validate_capability(endpoint);
    if (status != OP_STATUS_OK) {
        return status;
    }

    provider = op_usb_mode_select_provider(endpoint);
    if (provider == NULL || provider->set_mode == NULL) {
        return OP_STATUS_NOT_SUPPORTED;
    }

    status = provider->set_mode(endpoint, mode);

    if (status != OP_STATUS_OK) {
        return status;
    }

    endpoint->cached_mode = mode;
    op_usb_mode_endpoint_sync_to_controller(endpoint);
    return OP_STATUS_OK;
}

op_status_t op_video_chip_controller_get_usb_mode(op_video_chip_controller_t *controller, op_usb_mode_t *out_mode) {
    op_usb_mode_endpoint_t endpoint;

    if (controller == NULL || out_mode == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    op_usb_mode_endpoint_init(&endpoint);
    (void)op_usb_mode_endpoint_bind_chip_controller(&endpoint, controller);
    return op_usb_mode_get(&endpoint, out_mode);
}

op_status_t op_video_chip_controller_set_usb_mode(op_video_chip_controller_t *controller, op_usb_mode_t mode) {
    op_usb_mode_endpoint_t endpoint;

    if (controller == NULL) {
        return OP_STATUS_INVALID_ARGUMENT;
    }

    op_usb_mode_endpoint_init(&endpoint);
    (void)op_usb_mode_endpoint_bind_chip_controller(&endpoint, controller);
    return op_usb_mode_set(&endpoint, mode);
}
