#include "mock_transport.h"

#include <string.h>

static int mock_write(void *ctx, const uint8_t *data, size_t len) {
  mock_transport_t *mock = (mock_transport_t *)ctx;
  mock->write_calls++;
  if (mock->fail_write)
    return -1;

  // Truncate silently if a test ever writes more than the capture buffer holds;
  // real commands are far shorter than 4096 bytes.
  size_t room = sizeof(mock->written) - mock->written_len;
  size_t n = len < room ? len : room;
  memcpy(mock->written + mock->written_len, data, n);
  mock->written_len += n;
  return (int)len;
}

static int mock_read(void *ctx, uint8_t *buf, size_t len, int timeout_ms) {
  (void)timeout_ms; // the mock never blocks; it returns whatever is queued
  mock_transport_t *mock = (mock_transport_t *)ctx;
  mock->read_calls++;
  if (mock->fail_read)
    return -1;

  size_t avail = mock->response_len - mock->response_pos;
  if (avail == 0) {
    // Response exhausted. If a never-ending stream is configured, keep feeding
    // it cyclically (the port never goes quiet); otherwise report a timeout.
    if (mock->stream && mock->stream_len > 0) {
      size_t savail = mock->stream_len - mock->stream_pos;
      size_t n = savail < len ? savail : len;
      if (mock->read_chunk != 0 && n > mock->read_chunk)
        n = mock->read_chunk;
      memcpy(buf, mock->stream + mock->stream_pos, n);
      mock->stream_pos += n;
      if (mock->stream_pos >= mock->stream_len)
        mock->stream_pos = 0; // wrap -> unending
      return (int)n;
    }
    return 0; // exhausted -> looks like a timeout to the caller
  }

  size_t n = avail < len ? avail : len;
  if (mock->read_chunk != 0 && n > mock->read_chunk)
    n = mock->read_chunk;

  memcpy(buf, mock->response + mock->response_pos, n);
  mock->response_pos += n;
  return (int)n;
}

void mock_transport_init(mock_transport_t *mock) {
  memset(mock, 0, sizeof(*mock));
}

void mock_transport_set_response(mock_transport_t *mock, const uint8_t *data,
                                 size_t len) {
  mock->response = data;
  mock->response_len = len;
  mock->response_pos = 0;
}

void mock_transport_set_response_str(mock_transport_t *mock, const char *str) {
  mock_transport_set_response(mock, (const uint8_t *)str, strlen(str));
}

void mock_transport_set_stream_str(mock_transport_t *mock, const char *str) {
  mock->stream = (const uint8_t *)str;
  mock->stream_len = strlen(str);
  mock->stream_pos = 0;
}

neb_transport_t mock_transport_iface(mock_transport_t *mock) {
  neb_transport_t transport = {
      .write = mock_write,
      .read = mock_read,
      .ctx = mock,
  };
  return transport;
}

const char *mock_transport_written_str(mock_transport_t *mock) {
  size_t n = mock->written_len;
  if (n >= sizeof(mock->written))
    n = sizeof(mock->written) - 1;
  mock->written[n] = '\0';
  return (const char *)mock->written;
}

neb_handle_t mock_handle(mock_transport_t *mock, neb_model_t model) {
  neb_handle_t handle;
  memset(&handle, 0, sizeof(handle));
  handle.transport = mock_transport_iface(mock);
  handle.model = model;
  handle.caps = neb_caps_for_model(model);
  handle.timeout_ms = 50;
  handle.is_open = 1;
  return handle;
}
