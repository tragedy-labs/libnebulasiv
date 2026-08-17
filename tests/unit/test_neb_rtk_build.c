// test_neb_rtk_build.c
//
// Manual-as-spec tests for the RTK/correction command builders (Manual §4.5,
// §4.6). Exact wire strings, documented parameter bounds, invalid-parameter
// rejection. Pure builder output -- no handle, no transport.
#include "unity.h"

#include "build_assert.h"
#include "neb_protocol.h"
#include "neb_rtk.h"
#include "tests.h"

// Manual §4.5, Table 4-8 -- DGPS timeout; 0 disables, 1..1800 s
static void test_rtk_dgps_timeout(void) {
  BUILD_OK(neb_build_rtk_dgps_timeout(buf, sizeof(buf), 100),
           "CONFIG DGPS TIMEOUT 100"); // matches manual example
  BUILD_OK(neb_build_rtk_dgps_timeout(buf, sizeof(buf), 0),
           "CONFIG DGPS TIMEOUT 0"); // disable
  BUILD_OK(neb_build_rtk_dgps_timeout(buf, sizeof(buf), 1),
           "CONFIG DGPS TIMEOUT 1");
  BUILD_OK(neb_build_rtk_dgps_timeout(buf, sizeof(buf), 1800),
           "CONFIG DGPS TIMEOUT 1800"); // upper bound
  BUILD_ERR(neb_build_rtk_dgps_timeout(buf, sizeof(buf), 1801),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.6, Table 4-9 -- RTK timeout; 0 disables, 1..1800 s
static void test_rtk_timeout(void) {
  BUILD_OK(neb_build_rtk_timeout(buf, sizeof(buf), 60),
           "CONFIG RTK TIMEOUT 60"); // matches manual example
  BUILD_OK(neb_build_rtk_timeout(buf, sizeof(buf), 0), "CONFIG RTK TIMEOUT 0");
  BUILD_OK(neb_build_rtk_timeout(buf, sizeof(buf), 1800),
           "CONFIG RTK TIMEOUT 1800");
  BUILD_ERR(neb_build_rtk_timeout(buf, sizeof(buf), 1801),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.6, Table 4-10 -- RTK reliability; both thresholds 1..4
static void test_rtk_reliability(void) {
  BUILD_OK(neb_build_rtk_reliability(buf, sizeof(buf), 3, 1),
           "CONFIG RTK RELIABILITY 3 1"); // matches manual example
  BUILD_OK(neb_build_rtk_reliability(buf, sizeof(buf), 1, 1),
           "CONFIG RTK RELIABILITY 1 1"); // lower bounds
  BUILD_OK(neb_build_rtk_reliability(buf, sizeof(buf), 4, 4),
           "CONFIG RTK RELIABILITY 4 4"); // upper bounds
  BUILD_ERR(neb_build_rtk_reliability(buf, sizeof(buf), 0, 1),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_reliability(buf, sizeof(buf), 5, 1),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_reliability(buf, sizeof(buf), 3, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_reliability(buf, sizeof(buf), 3, 5),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.6 -- RTK solution-control tokens
static void test_rtk_solution_control(void) {
  BUILD_OK(neb_build_rtk_user_defaults(buf, sizeof(buf)),
           "CONFIG RTK USER_DEFAULTS");
  BUILD_OK(neb_build_rtk_reset(buf, sizeof(buf)), "CONFIG RTK RESET");
  BUILD_OK(neb_build_rtk_disable(buf, sizeof(buf)), "CONFIG RTK DISABLE");
}

// Manual §4.7, Table 4-11 -- STANDALONE: disable, default enable, coords, time
static void test_rtk_standalone(void) {
  BUILD_OK(neb_build_rtk_standalone_disable(buf, sizeof(buf)),
           "CONFIG STANDALONE DISABLE");
  BUILD_OK(neb_build_rtk_standalone_enable(buf, sizeof(buf)),
           "CONFIG STANDALONE ENABLE");

  // Coordinate form: 11 decimals lat/lon, 4 decimals alt.
  BUILD_OK(neb_build_rtk_standalone_enable_coords(buf, sizeof(buf), 1.5, 2.25,
                                                  57.23),
           "CONFIG STANDALONE ENABLE 1.50000000000 2.25000000000 57.2300");
  // Altitude ceiling is 18000 here (not 30000).
  {
    char buf[NEB_CMD_BUF_LEN];
    TEST_ASSERT_EQUAL_INT(NEB_OK, neb_build_rtk_standalone_enable_coords(
                                      buf, sizeof(buf), 90.0, 180.0, 18000.0));
    TEST_ASSERT_EQUAL_INT(
        NEB_OK, neb_build_rtk_standalone_enable_coords(buf, sizeof(buf), -90.0,
                                                       -180.0, -30000.0));
  }
  BUILD_ERR(neb_build_rtk_standalone_enable_coords(buf, sizeof(buf), 0, 0,
                                                   18000.0001),
            NEB_ERR_INVALID_PARAM); // altitude above 18000
  BUILD_ERR(
      neb_build_rtk_standalone_enable_coords(buf, sizeof(buf), 90.0001, 0, 0),
      NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_rtk_standalone_enable_coords(buf, sizeof(buf), 0, -180.0001, 0),
      NEB_ERR_INVALID_PARAM);

  // Time form: 3..100 s.
  BUILD_OK(neb_build_rtk_standalone_enable_time(buf, sizeof(buf), 60),
           "CONFIG STANDALONE ENABLE 60");
  BUILD_OK(neb_build_rtk_standalone_enable_time(buf, sizeof(buf), 3),
           "CONFIG STANDALONE ENABLE 3"); // min
  BUILD_OK(neb_build_rtk_standalone_enable_time(buf, sizeof(buf), 100),
           "CONFIG STANDALONE ENABLE 100"); // max
  BUILD_ERR(neb_build_rtk_standalone_enable_time(buf, sizeof(buf), 2),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_standalone_enable_time(buf, sizeof(buf), 101),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.10, Table 4-15 -- SBAS: every system token, disable, timeout range
static void test_rtk_sbas(void) {
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_AUTO),
           "CONFIG SBAS ENABLE Auto");
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_WAAS),
           "CONFIG SBAS ENABLE WAAS"); // matches manual example
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_GAGAN),
           "CONFIG SBAS ENABLE GAGAN");
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_MSAS),
           "CONFIG SBAS ENABLE MSAS");
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_EGNOS),
           "CONFIG SBAS ENABLE EGNOS");
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_SDCM),
           "CONFIG SBAS ENABLE SDCM");
  BUILD_OK(neb_build_rtk_sbas_enable(buf, sizeof(buf), NEB_SBAS_BDS),
           "CONFIG SBAS ENABLE BDS");
  BUILD_ERR(neb_build_rtk_sbas_enable(buf, sizeof(buf), (neb_sbas_system_t)999),
            NEB_ERR_INVALID_PARAM);

  BUILD_OK(neb_build_rtk_sbas_disable(buf, sizeof(buf)), "CONFIG SBAS DISABLE");

  BUILD_OK(neb_build_rtk_sbas_timeout(buf, sizeof(buf), 600),
           "CONFIG SBAS TIMEOUT 600"); // matches manual example
  BUILD_OK(neb_build_rtk_sbas_timeout(buf, sizeof(buf), 120),
           "CONFIG SBAS TIMEOUT 120"); // min
  BUILD_OK(neb_build_rtk_sbas_timeout(buf, sizeof(buf), 1800),
           "CONFIG SBAS TIMEOUT 1800"); // max
  BUILD_ERR(neb_build_rtk_sbas_timeout(buf, sizeof(buf), 119),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_sbas_timeout(buf, sizeof(buf), 1801),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.26, Table 4-34 -- base antenna: quoted name, sn, setup id, type,
// plus string validation and injection guards.
static void test_rtk_base_antenna(void) {
  // Exact manual example (name is quoted, may contain a space).
  BUILD_OK(neb_build_rtk_base_antenna(buf, sizeof(buf), "HXCCGX601A HXCS",
                                      "62815", 1, NEB_ANTENNA_USER),
           "CONFIG BASEANTENNAMODEL \"HXCCGX601A HXCS\" 62815 1 USER");
  // Documented defaults.
  BUILD_OK(neb_build_rtk_base_antenna(buf, sizeof(buf), "ADVNULLANTENNA",
                                      "a0001", 0, NEB_ANTENNA_NO),
           "CONFIG BASEANTENNAMODEL \"ADVNULLANTENNA\" a0001 0 NO");

  // setup_id bound 0..255.
  {
    char buf[NEB_CMD_BUF_LEN];
    TEST_ASSERT_EQUAL_INT(NEB_OK,
                          neb_build_rtk_base_antenna(buf, sizeof(buf), "N", "S",
                                                     255, NEB_ANTENNA_NO));
  }
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "N", "S", 256,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);

  // name length: 31 accepted, 32 rejected.
  {
    char buf[NEB_CMD_BUF_LEN];
    TEST_ASSERT_EQUAL_INT(
        NEB_OK, neb_build_rtk_base_antenna(buf, sizeof(buf),
                                           "1234567890123456789012345678901",
                                           "S", 0, NEB_ANTENNA_NO));
  }
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf),
                                       "12345678901234567890123456789012", "S",
                                       0, NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);

  // Empty / NULL name and sn are rejected.
  BUILD_ERR(
      neb_build_rtk_base_antenna(buf, sizeof(buf), "", "S", 0, NEB_ANTENNA_NO),
      NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_rtk_base_antenna(buf, sizeof(buf), "N", "", 0, NEB_ANTENNA_NO),
      NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), NULL, "S", 0,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "N", NULL, 0,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);

  // Injection guards: embedded quote or newline in name; space in sn.
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "bad\"name", "S", 0,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "bad\nname", "S", 0,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "N", "bad sn", 0,
                                       NEB_ANTENNA_NO),
            NEB_ERR_INVALID_PARAM);

  // Invalid type enum.
  BUILD_ERR(neb_build_rtk_base_antenna(buf, sizeof(buf), "N", "S", 0,
                                       (neb_antenna_type_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.22, Table 4-30 -- antenna height/offset, documented ranges
static void test_rtk_antenna_delta(void) {
  BUILD_OK(neb_build_rtk_antenna_delta(buf, sizeof(buf), 1.521, 0.0, 0.0),
           "CONFIG ANTENNADELTAHEN 1.5210 0.0000 0.0000"); // manual example
  BUILD_OK(neb_build_rtk_antenna_delta(buf, sizeof(buf), 0.0, 0.0, 0.0),
           "CONFIG ANTENNADELTAHEN 0.0000 0.0000 0.0000"); // defaults
  BUILD_OK(neb_build_rtk_antenna_delta(buf, sizeof(buf), 6.5535, 100.0, 100.0),
           "CONFIG ANTENNADELTAHEN 6.5535 100.0000 100.0000"); // upper bounds
  BUILD_ERR(neb_build_rtk_antenna_delta(buf, sizeof(buf), 6.5536, 0.0, 0.0),
            NEB_ERR_INVALID_PARAM); // height above 6.5535
  BUILD_ERR(neb_build_rtk_antenna_delta(buf, sizeof(buf), -0.0001, 0.0, 0.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_antenna_delta(buf, sizeof(buf), 0.0, 100.0001, 0.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_rtk_antenna_delta(buf, sizeof(buf), 0.0, 0.0, -0.0001),
            NEB_ERR_INVALID_PARAM);
}

void run_rtk_build_tests(void) {
  RUN_TEST(test_rtk_dgps_timeout);
  RUN_TEST(test_rtk_antenna_delta);
  RUN_TEST(test_rtk_timeout);
  RUN_TEST(test_rtk_reliability);
  RUN_TEST(test_rtk_solution_control);
  RUN_TEST(test_rtk_standalone);
  RUN_TEST(test_rtk_sbas);
  RUN_TEST(test_rtk_base_antenna);
}
