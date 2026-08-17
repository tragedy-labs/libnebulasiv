// mock_transport.h
//
// Hand-written fake neb_transport_t for unit tests. It captures everything the
// code under test writes, and feeds back a caller-supplied canned response on
// reads -- letting neb_send_command() be exercised for every outcome (OK, NAK,
// timeout, truncation, garbage) with no serial port involved.
#ifndef NEB_MOCK_TRANSPORT_H
#define NEB_MOCK_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "neb_core.h"

typedef struct {
  // Bytes the code under test wrote, concatenated across all write() calls.
  uint8_t written[4096];
  size_t written_len;

  // Canned bytes handed back by read(), consumed left to right.
  const uint8_t *response;
  size_t response_len;
  size_t response_pos;

  // Optional never-ending stream: once `response` is exhausted, read() keeps
  // returning these bytes cyclically instead of 0 (a quiet port). Models a
  // device with periodic output enabled -- the port never goes idle. NULL
  // (the default) means the port goes quiet after `response`.
  const uint8_t *stream;
  size_t stream_len;
  size_t stream_pos;

  // Test knobs.
  int fail_write;    // if nonzero, write() returns -1
  int fail_read;     // if nonzero, read() returns -1
  size_t read_chunk; // max bytes returned per read() (0 = as much as fits)
  int write_calls;   // number of write() calls made
  int read_calls;    // number of read() calls made
} mock_transport_t;

// Reset a mock to a clean state (no response, no failures, no chunk limit).
void mock_transport_init(mock_transport_t *mock);

// Set the canned response as raw bytes or as a C string (excluding the NUL).
void mock_transport_set_response(mock_transport_t *mock, const uint8_t *data,
                                 size_t len);
void mock_transport_set_response_str(mock_transport_t *mock, const char *str);

// Set a never-ending stream emitted cyclically once the response is exhausted
// (see the `stream` field). Pass a non-empty string to simulate a device that
// keeps outputting periodic data so the port never goes quiet.
void mock_transport_set_stream_str(mock_transport_t *mock, const char *str);

// A neb_transport_t bound to `mock` (which must outlive it).
neb_transport_t mock_transport_iface(mock_transport_t *mock);

// The captured writes as a NUL-terminated C string, for convenient assertions.
// Returns a pointer into `mock` valid until the next write.
const char *mock_transport_written_str(mock_transport_t *mock);

// Build an open handle wired to `mock` for `model`, with a short timeout so
// timeout tests run fast. No real port is opened.
neb_handle_t mock_handle(mock_transport_t *mock, neb_model_t model);

#endif
