// test_neb_admin_build.c
//
// Manual-as-spec tests for the administrative command builders (Manual §8):
// UNLOG, FRESET, RESET, SAVECONFIG. Exact wire strings, RESET flag
// combinations, and message validation. Pure builder output -- no handle, no
// transport.
#include "unity.h"

#include "build_assert.h"
#include "neb_admin.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §8.1 -- UNLOG, all four forms plus message validation
static void test_admin_unlog(void) {
  BUILD_OK(neb_build_admin_unlog_all(buf, sizeof(buf)), "UNLOG");
  BUILD_OK(neb_build_admin_unlog_message(buf, sizeof(buf), "GPGGA"),
           "UNLOG GPGGA");
  BUILD_OK(neb_build_admin_unlog_port(buf, sizeof(buf), NEB_COM1),
           "UNLOG COM1");
  BUILD_OK(
      neb_build_admin_unlog_port_message(buf, sizeof(buf), NEB_COM2, "GPGGA"),
      "UNLOG COM2 GPGGA");

  // Message validation: empty, NULL, embedded space, quote all rejected.
  BUILD_ERR(neb_build_admin_unlog_message(buf, sizeof(buf), ""),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_admin_unlog_message(buf, sizeof(buf), NULL),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_admin_unlog_message(buf, sizeof(buf), "GPGGA X"),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_admin_unlog_message(buf, sizeof(buf), "GP\"GA"),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_admin_unlog_port(buf, sizeof(buf), (neb_com_port_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §8.2 -- FRESET
static void test_admin_freset(void) {
  BUILD_OK(neb_build_admin_freset(buf, sizeof(buf)), "FRESET");
}

// Manual §8.3 -- RESET: bare, ALL, and flag combinations
static void test_admin_reset(void) {
  BUILD_OK(neb_build_admin_reset(buf, sizeof(buf)), "RESET");
  BUILD_OK(neb_build_admin_reset_all(buf, sizeof(buf)), "RESET ALL");
  BUILD_OK(neb_build_admin_reset_clear(buf, sizeof(buf), NEB_RESET_EPHEM),
           "RESET EPHEM");
  BUILD_OK(neb_build_admin_reset_clear(buf, sizeof(buf), NEB_RESET_POSITION),
           "RESET POSITION");
  // All targets, in the manual's example order.
  BUILD_OK(neb_build_admin_reset_clear(
               buf, sizeof(buf),
               NEB_RESET_EPHEM | NEB_RESET_ALMANAC | NEB_RESET_IONUTC |
                   NEB_RESET_POSITION | NEB_RESET_XOPARAM),
           "RESET EPHEM ALMANAC IONUTC POSITION XOPARAM");
  // Flag order is normalized regardless of argument order.
  BUILD_OK(neb_build_admin_reset_clear(buf, sizeof(buf),
                                       NEB_RESET_XOPARAM | NEB_RESET_EPHEM),
           "RESET EPHEM XOPARAM");
  // Empty flags and unknown bits are rejected.
  BUILD_ERR(neb_build_admin_reset_clear(buf, sizeof(buf), 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_admin_reset_clear(buf, sizeof(buf), 1u << 10),
            NEB_ERR_INVALID_PARAM);
}

// Manual §8.4 -- SAVECONFIG
static void test_admin_saveconfig(void) {
  BUILD_OK(neb_build_admin_saveconfig(buf, sizeof(buf)), "SAVECONFIG");
}

void run_admin_build_tests(void) {
  RUN_TEST(test_admin_unlog);
  RUN_TEST(test_admin_freset);
  RUN_TEST(test_admin_reset);
  RUN_TEST(test_admin_saveconfig);
}
