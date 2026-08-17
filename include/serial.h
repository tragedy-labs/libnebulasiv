// serial.h
//
// Minimal POSIX termios serial transport. This is the byte-level layer that
// neb_core sits on top of; nothing above neb_core should call into it
// directly.
#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct {
  int fd;
} serial_t;

// Open `device` at `baudrate` (8N1, raw). Returns 0 on success, -1 on error.
// Supported baud rates: 9600, 19200, 38400, 57600, 115200, 230400, 460800,
// 921600.
int serial_open(serial_t *serial, const char *device, int baudrate);

// Close the port. Safe to call on an already-closed handle.
void serial_close(serial_t *serial);

// Blocking-until-data read. Returns bytes read, 0 if none were available, or
// -1 on error. Provided for compatibility; prefer serial_read_timed() when a
// bounded wait is needed.
ssize_t serial_read(serial_t *serial, uint8_t *buffer, size_t length);

// Read up to `length` bytes, waiting at most `timeout_ms` for the first byte
// to arrive. Returns the number of bytes read (>0), 0 on timeout with no data,
// or -1 on error. This is the primitive neb_core uses to implement command
// response timeouts.
ssize_t serial_read_timed(serial_t *serial, uint8_t *buffer, size_t length,
                          int timeout_ms);

// Write exactly `length` bytes, retrying on partial writes. Returns bytes
// written on success, or -1 on error.
ssize_t serial_write(serial_t *serial, const uint8_t *buffer, size_t length);

// Discard any bytes currently buffered on the input side (best effort).
void serial_flush_input(serial_t *serial);

#endif
