#include <stdio.h>
#include <string.h>

#include "openterface/transport.h"
#include "test_helpers.h"

int test_failures = 0;

/* -- mock transport context ------------------------------------------─ */

typedef struct {
    int open_called;
    int close_called;
    int read_called;
    int write_called;
    int control_called;
    int open_succeeds;
    int close_succeeds;
    op_status_t read_status;
    op_status_t write_status;
    op_status_t control_status;
    size_t read_bytes;
    uint8_t read_buffer[64];
    uint8_t write_buffer[64];
    size_t write_length;
} mock_context_t;

static op_status_t mock_open(void *context) {
    mock_context_t *mock = (mock_context_t *)context;
    mock->open_called++;
    return mock->open_succeeds ? OP_STATUS_OK : OP_STATUS_IO_ERROR;
}

static op_status_t mock_close(void *context) {
    mock_context_t *mock = (mock_context_t *)context;
    mock->close_called++;
    return mock->close_succeeds ? OP_STATUS_OK : OP_STATUS_IO_ERROR;
}

static op_status_t mock_read(void *context, uint8_t *buffer, size_t capacity, size_t *out_length) {
    mock_context_t *mock = (mock_context_t *)context;
    mock->read_called++;
    if (mock->read_status != OP_STATUS_OK) {
        return mock->read_status;
    }
    size_t copy_len = mock->read_bytes < capacity ? mock->read_bytes : capacity;
    memcpy(buffer, mock->read_buffer, copy_len);
    *out_length = copy_len;
    return OP_STATUS_OK;
}

static op_status_t mock_write(void *context, const uint8_t *buffer, size_t length) {
    mock_context_t *mock = (mock_context_t *)context;
    mock->write_called++;
    if (mock->write_status != OP_STATUS_OK) {
        return mock->write_status;
    }
    memcpy(mock->write_buffer, buffer, length);
    mock->write_length = length;
    return OP_STATUS_OK;
}

static op_status_t mock_control(void *context, uint32_t request, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_capacity, size_t *out_length) {
    mock_context_t *mock = (mock_context_t *)context;
    mock->control_called++;
    (void)request; (void)input; (void)input_length; (void)output; (void)output_capacity;
    if (mock->control_status != OP_STATUS_OK) {
        return mock->control_status;
    }
    *out_length = 0;
    return OP_STATUS_OK;
}

static void init_mock(mock_context_t *mock) {
    memset(mock, 0, sizeof(*mock));
    mock->open_succeeds = 1;
    mock->close_succeeds = 1;
    mock->read_status = OP_STATUS_OK;
    mock->write_status = OP_STATUS_OK;
    mock->control_status = OP_STATUS_OK;
}

static void init_mock_vtable(op_transport_vtable_t *vtable) {
    vtable->open = mock_open;
    vtable->close = mock_close;
    vtable->read = mock_read;
    vtable->write = mock_write;
    vtable->control = mock_control;
}

/* -- test: init ------------------------------------------------------─ */

static void test_init(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;

    init_mock(&mock);
    init_mock_vtable(&vtable);

    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    ASSERT_EQ_INT(OP_TRANSPORT_KIND_SERIAL, transport.kind, "kind set");
    ASSERT_TRUE(transport.context == &mock, "context set");
    ASSERT_TRUE(transport.vtable == &vtable, "vtable set");
    ASSERT_EQ_INT(0, transport.is_open, "initially closed");
}

static void test_init_null(void) {
    mock_context_t mock;
    op_transport_vtable_t vtable;

    init_mock(&mock);
    init_mock_vtable(&vtable);

    /* Should not crash */
    op_transport_init(NULL, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);
}

/* -- test: open/close ------------------------------------------------─ */

static void test_open_close(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    /* Open */
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_open(&transport), "open succeeds");
    ASSERT_EQ_INT(1, transport.is_open, "is_open set");
    ASSERT_EQ_INT(1, mock.open_called, "open called once");

    /* Close */
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_close(&transport), "close succeeds");
    ASSERT_EQ_INT(0, transport.is_open, "is_open cleared");
    ASSERT_EQ_INT(1, mock.close_called, "close called once");
}

static void test_open_fails(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    mock.open_succeeds = 0;
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, op_transport_open(&transport), "open fails");
    ASSERT_EQ_INT(0, transport.is_open, "is_open not set on failure");
}

static void test_close_fails(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    mock.close_succeeds = 0;
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    /* Open first */
    op_transport_open(&transport);
    ASSERT_EQ_INT(1, transport.is_open, "opened");

    /* Close fails */
    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, op_transport_close(&transport), "close fails");
    ASSERT_EQ_INT(1, transport.is_open, "is_open still set on failure");
}

/* -- test: read/write ------------------------------------------------─ */

static void test_read_write(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;
    uint8_t buffer[32];
    size_t out_length;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    /* Write */
    uint8_t data[] = {0x01, 0x02, 0x03};
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_write(&transport, data, 3), "write succeeds");
    ASSERT_EQ_INT(1, mock.write_called, "write called once");
    ASSERT_EQ_INT(3, mock.write_length, "write length");
    ASSERT_EQ_INT(0x01, mock.write_buffer[0], "write data[0]");
    ASSERT_EQ_INT(0x02, mock.write_buffer[1], "write data[1]");
    ASSERT_EQ_INT(0x03, mock.write_buffer[2], "write data[2]");

    /* Read */
    mock.read_buffer[0] = 0xAA;
    mock.read_buffer[1] = 0xBB;
    mock.read_bytes = 2;
    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_read(&transport, buffer, sizeof(buffer), &out_length), "read succeeds");
    ASSERT_EQ_INT(1, mock.read_called, "read called once");
    ASSERT_EQ_INT(2, out_length, "read length");
    ASSERT_EQ_INT(0xAA, buffer[0], "read data[0]");
    ASSERT_EQ_INT(0xBB, buffer[1], "read data[1]");
}

static void test_read_write_errors(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;
    uint8_t buffer[32];
    size_t out_length;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    mock.read_status = OP_STATUS_TIMEOUT;
    mock.write_status = OP_STATUS_IO_ERROR;
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    ASSERT_EQ_INT(OP_STATUS_IO_ERROR, op_transport_write(&transport, buffer, 1), "write error");
    ASSERT_EQ_INT(OP_STATUS_TIMEOUT, op_transport_read(&transport, buffer, sizeof(buffer), &out_length), "read error");
}

/* -- test: control ---------------------------------------------------- */

static void test_control(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;
    uint8_t input[] = {0x01, 0x02};
    uint8_t output[16];
    size_t out_length;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    ASSERT_EQ_INT(OP_STATUS_OK, op_transport_control(&transport, 0x1234, input, 2, output, sizeof(output), &out_length), "control succeeds");
    ASSERT_EQ_INT(1, mock.control_called, "control called once");
}

static void test_control_error(void) {
    op_transport_t transport;
    mock_context_t mock;
    op_transport_vtable_t vtable;
    uint8_t output[16];
    size_t out_length;

    init_mock(&mock);
    init_mock_vtable(&vtable);
    mock.control_status = OP_STATUS_NOT_SUPPORTED;
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, &vtable);

    ASSERT_EQ_INT(OP_STATUS_NOT_SUPPORTED, op_transport_control(&transport, 0, NULL, 0, output, sizeof(output), &out_length), "control error");
}

/* -- test: null safety ------------------------------------------------ */

static void test_null_safety(void) {
    uint8_t buffer[16];
    size_t out_length;

    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_open(NULL), "open null");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_close(NULL), "close null");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_read(NULL, buffer, sizeof(buffer), &out_length), "read null");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_write(NULL, buffer, 1), "write null");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_control(NULL, 0, NULL, 0, buffer, sizeof(buffer), &out_length), "control null");
}

static void test_null_vtable(void) {
    op_transport_t transport;
    mock_context_t mock;
    uint8_t buffer[16];
    size_t out_length;

    init_mock(&mock);
    op_transport_init(&transport, OP_TRANSPORT_KIND_SERIAL, &mock, NULL);

    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_open(&transport), "open null vtable");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_close(&transport), "close null vtable");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_read(&transport, buffer, sizeof(buffer), &out_length), "read null vtable");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_write(&transport, buffer, 1), "write null vtable");
    ASSERT_EQ_INT(OP_STATUS_INVALID_ARGUMENT, op_transport_control(&transport, 0, NULL, 0, buffer, sizeof(buffer), &out_length), "control null vtable");
}

/* -- test: kind labels ------------------------------------------------ */

static void test_kind_labels(void) {
    ASSERT_EQ_STR("serial", op_transport_kind_label(OP_TRANSPORT_KIND_SERIAL), "serial label");
    ASSERT_EQ_STR("hid_feature", op_transport_kind_label(OP_TRANSPORT_KIND_HID_FEATURE), "hid_feature label");
    ASSERT_EQ_STR("web_serial", op_transport_kind_label(OP_TRANSPORT_KIND_WEB_SERIAL), "web_serial label");
    ASSERT_EQ_STR("web_hid", op_transport_kind_label(OP_TRANSPORT_KIND_WEB_HID), "web_hid label");
    ASSERT_EQ_STR("local_service", op_transport_kind_label(OP_TRANSPORT_KIND_LOCAL_SERVICE), "local_service label");
    ASSERT_EQ_STR("ble", op_transport_kind_label(OP_TRANSPORT_KIND_BLE), "ble label");
    ASSERT_EQ_STR("stub", op_transport_kind_label(OP_TRANSPORT_KIND_STUB), "stub label");
    ASSERT_EQ_STR("unknown", op_transport_kind_label(OP_TRANSPORT_KIND_UNKNOWN), "unknown label");
    ASSERT_EQ_STR("unknown", op_transport_kind_label(999), "invalid label");
}

int main(void) {
    printf("Running transport tests...\n");

    RUN_TEST(test_init);
    RUN_TEST(test_init_null);
    RUN_TEST(test_open_close);
    RUN_TEST(test_open_fails);
    RUN_TEST(test_close_fails);
    RUN_TEST(test_read_write);
    RUN_TEST(test_read_write_errors);
    RUN_TEST(test_control);
    RUN_TEST(test_control_error);
    RUN_TEST(test_null_safety);
    RUN_TEST(test_null_vtable);
    RUN_TEST(test_kind_labels);

    if (test_failures != 0) {
        fprintf(stderr, "transport_test: %d failure(s)\n", test_failures);
        return 1;
    }

    printf("transport_test: all tests passed\n");
    return 0;
}
