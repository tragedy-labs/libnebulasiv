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
  mock_transport_set_response_str(&mock, "$command,mode,response: OK*00\r\n");
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
}
