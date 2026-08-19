// test_neb_send_command.c
//
// Tests for neb_send_command()'s response parsing and status mapping, driven
// through the hand-written mock transport with real UM980 captures (see
// fixtures/um980_session.h). Covers: OK, NAK, timeout, garbage, truncated
// response buffer, chunked delivery, and I/O failures.
#include "unity.h"

#include <string.h>

#include "fixtures/um980_session.h"
#include "mock_transport.h"
#include "neb_core.h"
#include "tests.h"

// Framing: a command is written verbatim with a trailing CR+LF.
static void test_send_writes_framing(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_MODE_QUERY_OK);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  neb_send_command(&h, "MODE", NULL, 0);
  TEST_ASSERT_EQUAL_STRING("MODE\r\n", mock_transport_written_str(&mock));
}

// OK ack -> NEB_OK, and the query payload is captured into the response buffer.
static void test_send_ok_captures_payload(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_MODE_QUERY_OK);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  char resp[256];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_send_command(&h, "MODE", resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "response: OK"));
  TEST_ASSERT_NOT_NULL(strstr(resp, "#MODE,")); // payload line captured
}

// OK works with no response buffer (setter commands pass NULL).
static void test_send_ok_null_response(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_MODE_QUERY_OK);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_send_command(&h, "MODE", NULL, 0));
}

// "PARSING FAILED" acks map to NEB_ERR_NAK, regardless of the specific reason.
static void test_send_nak_missing_field(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_NAK_MISSING_FIELD);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_ERR_NAK,
                        neb_send_command(&h, "CONFIG COM9 115200", NULL, 0));
}

static void test_send_nak_no_matching_func(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_NAK_NO_FUNC);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(
      NEB_ERR_NAK, neb_send_command(&h, "THISHEADERDOESNOTEXIST", NULL, 0));
}

// No reply at all -> timeout.
static void test_send_timeout_no_reply(void) {
  mock_transport_t mock;
  mock_transport_init(&mock); // empty response
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_ERR_TIMEOUT, neb_send_command(&h, "MODE", NULL, 0));
}

// Bytes arrive but never form a "$command,...,response:" ack -> timeout.
static void test_send_timeout_garbage(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock,
                                  "noise without an ack\r\n$GPGGA,junk\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_ERR_TIMEOUT, neb_send_command(&h, "MODE", NULL, 0));
}

// Stray leading bytes before the ack are tolerated (prefix scan).
static void test_send_ok_with_garbage_prefix(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_OK_WITH_GARBAGE_PREFIX);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_send_command(&h, "unlog", NULL, 0));
}

// A response buffer smaller than the reply is filled safely and NUL-terminated
// (ASan/UBSan in the test build would catch any overrun here).
static void test_send_truncated_response(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_MODE_QUERY_OK);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  char resp[16];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_send_command(&h, "MODE", resp, sizeof(resp)));
  TEST_ASSERT_TRUE(strlen(resp) < sizeof(resp)); // never overruns
}

// Delivery split into tiny chunks still parses and still captures the payload
// (exercises the same-chunk tail capture and the multi-chunk drain).
static void test_send_chunked_delivery(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, UM980_FIX_MODE_QUERY_OK);
  mock.read_chunk = 4; // force many small reads
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  char resp[256];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_send_command(&h, "MODE", resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "#MODE,"));
}

// A query against a device that is streaming periodic output must still
// return: the drain loop is bounded by both the response-buffer size and an
// overall time budget, so a port that never goes quiet cannot hang it. Before
// that bound existed this call looped forever. We assert it returns promptly
// (bounded read count) with the OK status and a safely terminated buffer.
static void test_send_query_during_stream_terminates(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  // Ack arrives, then the device keeps emitting periodic NMEA forever.
  mock_transport_set_response_str(&mock, "$command,mode,response: OK*5D\r\n");
  mock_transport_set_stream_str(
      &mock, "$GPGGA,000000.00,,,,,0,00,99.99,,,,,,*48\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  char resp[64];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_send_command(&h, "MODE", resp, sizeof(resp)));
  TEST_ASSERT_TRUE(strlen(resp) < sizeof(resp)); // safely terminated
  // The buffer-full guard ends the drain in a handful of reads -- nowhere near
  // an unbounded spin.
  TEST_ASSERT_TRUE(mock.read_calls < 100);
}

// Transport write failure -> NEB_ERR_IO.
static void test_send_write_failure(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock.fail_write = 1;
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_ERR_IO, neb_send_command(&h, "MODE", NULL, 0));
}

// Transport read failure -> NEB_ERR_IO.
static void test_send_read_failure(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock.fail_read = 1;
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  TEST_ASSERT_EQUAL_INT(NEB_ERR_IO, neb_send_command(&h, "MODE", NULL, 0));
}

// Unopened / NULL handles are rejected before any transport call.
static void test_send_invalid_handle(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  h.is_open = 0;
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_HANDLE,
                        neb_send_command(&h, "MODE", NULL, 0));
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_HANDLE,
                        neb_send_command(NULL, "MODE", NULL, 0));
  TEST_ASSERT_EQUAL_INT(0, mock.write_calls); // nothing was sent
}

// --- Acknowledgement checksum ---------------------------------------------
// "*hh" is the XOR of every character from the leading '$' through the
// character before the '*', including the '$'. Undocumented in the manual;
// confirmed against captured device bytes (see fixtures/um980_session.h).

// A correct checksum is accepted -- the value here is the one a real UM980
// returned for this exact line.
static void test_send_ack_checksum_valid(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,MODE,response: OK*5D\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_send_command(&h, "MODE", NULL, 0));
}

// A corrupted checksum is line noise, not a rejection: the line is skipped and
// the scan continues, so a good ack arriving afterwards still counts.
static void test_send_ack_checksum_corrupt_is_skipped(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,MODE,response: OK*FF\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_ERR_TIMEOUT, neb_send_command(&h, "MODE", NULL, 0));

  // Same corrupt line, then a good one: the good one wins.
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,MODE,response: OK*FF\r\n"
                                         "$command,MODE,response: OK*5D\r\n");
  neb_handle_t h2 = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_send_command(&h2, "MODE", NULL, 0));
}

// A corrupt checksum must not turn a rejection into a success or vice versa:
// a NAK line with a valid checksum is still a NAK.
static void test_send_ack_checksum_valid_nak(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  // Captured verbatim from a real UM980 (fixtures/um980_session.h).
  mock_transport_set_response_str(
      &mock,
      "$command,config com9 115200,response: PARSING FAILED MISSING FIELD,*56"
      "\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_ERR_NAK,
                        neb_send_command(&h, "config com9 115200", NULL, 0));
}

// A reply carrying no "*" field at all is still accepted: not every reply form
// is known to include a checksum, and refusing a well-formed acknowledgement
// over a missing one would be the worse failure.
static void test_send_ack_without_checksum_accepted(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,MODE,response: OK\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_send_command(&h, "MODE", NULL, 0));
}

// --- neb_read_raw ----------------------------------------------------------
// Raw stream reading: no framing, no ack parsing. Used for the output messages
// the receiver produces once logging is turned on (RTCM3, NMEA, §7 logs).

static void test_read_raw_returns_bytes(void) {
  // An RTCM3 frame header and payload: binary, with embedded NUL bytes. Set
  // with an explicit length, not as a C string -- a strlen-based helper would
  // stop at the first NUL and quietly test almost nothing, and surviving NULs
  // is the whole reason this entry point exists.
  static const uint8_t frame[] = {0xd3, 0x00, 0x13, 0x3e, 0x00, 0x00, 0x7f};
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response(&mock, frame, sizeof(frame));
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  uint8_t buffer[64];
  size_t received = 0;
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_read_raw(&h, buffer, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_UINT(sizeof(frame), received);
  // Handed through byte for byte: no framing, no NUL truncation, no rewriting.
  TEST_ASSERT_EQUAL_UINT8_ARRAY(frame, buffer, sizeof(frame));
}

// A quiet port is a timeout, not an error -- a streaming loop treats it as
// "nothing yet" and goes round again.
static void test_read_raw_quiet_port_times_out(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  uint8_t buffer[64];
  size_t received = 99;
  TEST_ASSERT_EQUAL_INT(NEB_ERR_TIMEOUT,
                        neb_read_raw(&h, buffer, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_UINT(0, received);
}

static void test_read_raw_transport_failure(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock.fail_read = 1;
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  uint8_t buffer[64];
  size_t received = 99;
  TEST_ASSERT_EQUAL_INT(NEB_ERR_IO,
                        neb_read_raw(&h, buffer, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_UINT(0, received);
}

static void test_read_raw_rejects_bad_arguments(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "bytes");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);

  uint8_t buffer[64];
  size_t received = 0;
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_PARAM,
                        neb_read_raw(&h, NULL, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_PARAM,
                        neb_read_raw(&h, buffer, 0, 10, &received));
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_PARAM,
                        neb_read_raw(&h, buffer, sizeof(buffer), 10, NULL));

  neb_handle_t closed = mock_handle(&mock, NEB_MODEL_UM980);
  closed.is_open = 0;
  TEST_ASSERT_EQUAL_INT(
      NEB_ERR_INVALID_HANDLE,
      neb_read_raw(&closed, buffer, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_INT(
      NEB_ERR_INVALID_HANDLE,
      neb_read_raw(NULL, buffer, sizeof(buffer), 10, &received));
  TEST_ASSERT_EQUAL_INT(0, mock.read_calls); // nothing reached the transport
}

void run_send_command_tests(void) {
  RUN_TEST(test_send_writes_framing);
  RUN_TEST(test_send_ok_captures_payload);
  RUN_TEST(test_send_ok_null_response);
  RUN_TEST(test_send_nak_missing_field);
  RUN_TEST(test_send_nak_no_matching_func);
  RUN_TEST(test_send_timeout_no_reply);
  RUN_TEST(test_send_timeout_garbage);
  RUN_TEST(test_send_ok_with_garbage_prefix);
  RUN_TEST(test_send_truncated_response);
  RUN_TEST(test_send_chunked_delivery);
  RUN_TEST(test_send_query_during_stream_terminates);
  RUN_TEST(test_send_write_failure);
  RUN_TEST(test_send_read_failure);
  RUN_TEST(test_send_invalid_handle);
  RUN_TEST(test_send_ack_checksum_valid);
  RUN_TEST(test_send_ack_checksum_corrupt_is_skipped);
  RUN_TEST(test_send_ack_checksum_valid_nak);
  RUN_TEST(test_send_ack_without_checksum_accepted);
  RUN_TEST(test_read_raw_returns_bytes);
  RUN_TEST(test_read_raw_quiet_port_times_out);
  RUN_TEST(test_read_raw_transport_failure);
  RUN_TEST(test_read_raw_rejects_bad_arguments);
}
