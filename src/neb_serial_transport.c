#include "neb_serial_transport.h"

// Thin adapters from the neb_transport_t signatures to the serial API. The
// interface uses int returns; the serial layer's ssize_t results fit (byte
// counts are small and errors are negative).
static int serial_transport_write(void *ctx, const uint8_t *data, size_t len) {
  return (int)serial_write((serial_t *)ctx, data, len);
}

static int serial_transport_read(void *ctx, uint8_t *buf, size_t len,
                                 int timeout_ms) {
  return (int)serial_read_timed((serial_t *)ctx, buf, len, timeout_ms);
}

neb_transport_t neb_serial_transport(serial_t *serial) {
  neb_transport_t transport = {
      .write = serial_transport_write,
      .read = serial_transport_read,
      .ctx = serial,
  };
  return transport;
}
