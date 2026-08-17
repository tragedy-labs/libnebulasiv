// test_neb_logging_build.c
//
// Manual-as-spec tests for the data-output (logging) command builders
// (Manual §7). They pin the exact command structure -- message + optional port
// + optional period/ONCHANGED -- against the manual's examples, plus message
// and period validation. Pure builder output; no handle, no transport.
#include "unity.h"

#include "build_assert.h"
#include "mock_transport.h"
#include "neb_logging.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §7 -- output once (no period)
static void test_logging_once(void) {
  BUILD_OK(neb_build_logging_once(buf, sizeof(buf), "VERSIONA"),
           "VERSIONA"); // matches the manual example
  BUILD_OK(neb_build_logging_once_port(buf, sizeof(buf), "GPGGA", NEB_COM2),
           "GPGGA COM2");

  // Message validation.
  BUILD_ERR(neb_build_logging_once(buf, sizeof(buf), ""),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_logging_once(buf, sizeof(buf), NULL),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_logging_once(buf, sizeof(buf), "GPGGA 1"),
            NEB_ERR_INVALID_PARAM); // embedded space
  BUILD_ERR(neb_build_logging_once_port(buf, sizeof(buf), "GPGGA",
                                        (neb_com_port_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §7 -- periodic output; period in seconds (1=1Hz, 0.5=2Hz, ...)
static void test_logging_periodic(void) {
  BUILD_OK(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", 1.0),
           "GPGGA 1"); // matches manual example
  BUILD_OK(neb_build_logging_periodic(buf, sizeof(buf), "GPDTM", 1.0),
           "GPDTM 1"); // matches manual example
  BUILD_OK(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", 0.5),
           "GPGGA 0.5"); // 2 Hz
  BUILD_OK(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", 0.2),
           "GPGGA 0.2"); // 5 Hz
  BUILD_OK(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", 0.1),
           "GPGGA 0.1"); // 10 Hz
  BUILD_OK(
      neb_build_logging_periodic_port(buf, sizeof(buf), "GPGGA", NEB_COM2, 1.0),
      "GPGGA COM2 1"); // matches manual example

  // Period must be > 0.
  BUILD_ERR(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", 0.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_logging_periodic(buf, sizeof(buf), "GPGGA", -1.0),
            NEB_ERR_INVALID_PARAM);
}

// Manual §7 -- ONCHANGED trigger
static void test_logging_onchanged(void) {
  BUILD_OK(neb_build_logging_onchanged(buf, sizeof(buf), "GPSIONA"),
           "GPSIONA ONCHANGED"); // matches manual example
  BUILD_OK(
      neb_build_logging_onchanged_port(buf, sizeof(buf), "OBSVBASEA", NEB_COM1),
      "OBSVBASEA COM1 ONCHANGED"); // matches manual example
  BUILD_ERR(neb_build_logging_onchanged(buf, sizeof(buf), ""),
            NEB_ERR_INVALID_PARAM);
}

// Manual §7.1/§7.2 -- every NMEA message enum maps to its exact wire name.
static void test_nmea_message_str(void) {
  static const struct {
    neb_nmea_message_t msg;
    const char *token;
  } cases[] = {
      {NEB_NMEA_GPDTM, "GPDTM"},   {NEB_NMEA_GPGBS, "GPGBS"},
      {NEB_NMEA_GPGGA, "GPGGA"},   {NEB_NMEA_GPGLL, "GPGLL"},
      {NEB_NMEA_GPGNS, "GPGNS"},   {NEB_NMEA_GPGRS, "GPGRS"},
      {NEB_NMEA_GPGSA, "GPGSA"},   {NEB_NMEA_GPGST, "GPGST"},
      {NEB_NMEA_GPGSV, "GPGSV"},   {NEB_NMEA_GPRMC, "GPRMC"},
      {NEB_NMEA_GPROT, "GPROT"},   {NEB_NMEA_GPTHS, "GPTHS"},
      {NEB_NMEA_GPVTG, "GPVTG"},   {NEB_NMEA_GPZDA, "GPZDA"},
      {NEB_NMEA_GPGGAH, "GPGGAH"}, {NEB_NMEA_GPGLLH, "GPGLLH"},
      {NEB_NMEA_GPGNSH, "GPGNSH"}, {NEB_NMEA_GPGRSH, "GPGRSH"},
      {NEB_NMEA_GPGSAH, "GPGSAH"}, {NEB_NMEA_GPGSTH, "GPGSTH"},
      {NEB_NMEA_GPGSVH, "GPGSVH"}, {NEB_NMEA_GPRMCH, "GPRMCH"},
      {NEB_NMEA_GPVTGH, "GPVTGH"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    TEST_ASSERT_EQUAL_STRING(cases[i].token,
                             neb_nmea_message_str(cases[i].msg));
  TEST_ASSERT_NULL(neb_nmea_message_str((neb_nmea_message_t)999));
}

// The typed wrappers resolve the enum and produce the right wire command.
static void test_nmea_typed_wrappers(void) {
  mock_transport_t mock;
  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,x,response: OK*00\r\n");
  neb_handle_t h = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_logging_nmea_periodic(&h, NEB_NMEA_GPGGA, 1.0));
  TEST_ASSERT_EQUAL_STRING("GPGGA 1\r\n", mock_transport_written_str(&mock));

  mock_transport_init(&mock);
  mock_transport_set_response_str(&mock, "$command,x,response: OK*00\r\n");
  neb_handle_t h2 = mock_handle(&mock, NEB_MODEL_UM982);
  TEST_ASSERT_EQUAL_INT(
      NEB_OK, neb_logging_nmea_onchanged_port(&h2, NEB_NMEA_GPGSVH, NEB_COM1));
  TEST_ASSERT_EQUAL_STRING("GPGSVH COM1 ONCHANGED\r\n",
                           mock_transport_written_str(&mock));

  // Out-of-range enum is rejected before any transport call.
  mock_transport_init(&mock);
  neb_handle_t h3 = mock_handle(&mock, NEB_MODEL_UM980);
  TEST_ASSERT_EQUAL_INT(NEB_ERR_INVALID_PARAM,
                        neb_logging_nmea_once(&h3, (neb_nmea_message_t)999));
  TEST_ASSERT_EQUAL_INT(0, mock.write_calls);
}

void run_logging_build_tests(void) {
  RUN_TEST(test_logging_once);
  RUN_TEST(test_logging_periodic);
  RUN_TEST(test_logging_onchanged);
  RUN_TEST(test_nmea_message_str);
  RUN_TEST(test_nmea_typed_wrappers);
}
