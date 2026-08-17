// test_neb_mode_build.c
//
// Manual-as-spec tests for the MODE command builders (Manual §3). Each test
// pins the exact wire string documented in the manual -- keyword casing,
// spacing, ordering -- plus every enum token, documented parameter bounds, and
// invalid-parameter rejection. No handle, no transport: pure builder output.
#include "unity.h"

#include "build_assert.h"
#include "neb_mode.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §3.1 -- MODE query
static void test_mode_query(void) {
  BUILD_OK(neb_build_mode_query(buf, sizeof(buf)), "MODE");
}

// Manual §3.6 -- MODE ROVER (model default)
static void test_mode_rover_default(void) {
  BUILD_OK(neb_build_mode_set_rover(buf, sizeof(buf)), "MODE ROVER");
}

// Manual §3.6, Table 3-8 -- every rover profile token, plus invalid enum
static void test_mode_rover_profiles(void) {
  BUILD_OK(neb_build_mode_set_rover_profile(buf, sizeof(buf), NEB_ROVER_UAV),
           "MODE ROVER UAV");
  BUILD_OK(neb_build_mode_set_rover_profile(buf, sizeof(buf),
                                            NEB_ROVER_UAV_FORMATION),
           "MODE ROVER UAV FORMATION");
  BUILD_OK(neb_build_mode_set_rover_profile(buf, sizeof(buf), NEB_ROVER_SURVEY),
           "MODE ROVER SURVEY");
  BUILD_OK(
      neb_build_mode_set_rover_profile(buf, sizeof(buf), NEB_ROVER_SURVEY_MOW),
      "MODE ROVER SURVEY MOW");
  BUILD_OK(
      neb_build_mode_set_rover_profile(buf, sizeof(buf), NEB_ROVER_AUTOMOTIVE),
      "MODE ROVER AUTOMOTIVE");
  BUILD_ERR(neb_build_mode_set_rover_profile(buf, sizeof(buf),
                                             (neb_rover_profile_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §3.4 -- MODE BASE (default self-survey)
static void test_mode_base_auto(void) {
  BUILD_OK(neb_build_mode_set_base_auto(buf, sizeof(buf)), "MODE BASE");
}

// Manual §3.5, Table 3-7 -- base ID, valid range 0..4095
static void test_mode_base_id(void) {
  BUILD_OK(neb_build_mode_set_base_id(buf, sizeof(buf), 0), "MODE BASE 0");
  BUILD_OK(neb_build_mode_set_base_id(buf, sizeof(buf), 2048),
           "MODE BASE 2048");
  BUILD_OK(neb_build_mode_set_base_id(buf, sizeof(buf), 4095),
           "MODE BASE 4095");
  BUILD_ERR(neb_build_mode_set_base_id(buf, sizeof(buf), 4096),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_id(buf, sizeof(buf), 65535),
            NEB_ERR_INVALID_PARAM);
}

// Manual §3.3 -- self-optimizing base, time only
static void test_mode_base_self_optimize(void) {
  BUILD_OK(neb_build_mode_set_base_self_optimize(buf, sizeof(buf), 60),
           "MODE BASE TIME 60");
  BUILD_OK(neb_build_mode_set_base_self_optimize(buf, sizeof(buf), 0),
           "MODE BASE TIME 0");
}

// Manual §3.3, Table 3-5 -- self-optimizing base with distance, range 0..10 m
static void test_mode_base_self_optimize_dist(void) {
  BUILD_OK(
      neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 60, 5.0),
      "MODE BASE TIME 60 5"); // matches manual example
  BUILD_OK(
      neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 60, 0.0),
      "MODE BASE TIME 60 0");
  BUILD_OK(
      neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 60, 10.0),
      "MODE BASE TIME 60 10"); // upper bound
  BUILD_OK(neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 0, 2.5),
           "MODE BASE TIME 0 2.5");
  BUILD_ERR(
      neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 60, -1.0),
      NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_mode_set_base_self_optimize_dist(buf, sizeof(buf), 60, 10.5),
      NEB_ERR_INVALID_PARAM);
}

// Manual §3.2, Table 3-4 -- fixed base coordinates, documented bounds
static void test_mode_base_fixed(void) {
  // Exact format: "MODE BASE", %.11f lat/lon, %.4f alt.
  BUILD_OK(
      neb_build_mode_set_base_fixed(buf, sizeof(buf), 12.5, 45.25, 58.0984),
      "MODE BASE 12.50000000000 45.25000000000 58.0984");

  // Boundaries are accepted.
  char buf[NEB_CMD_BUF_LEN];
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_build_mode_set_base_fixed(
                                    buf, sizeof(buf), 90.0, 180.0, 30000.0));
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_build_mode_set_base_fixed(
                                    buf, sizeof(buf), -90.0, -180.0, -30000.0));

  // Just outside each documented bound is rejected.
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), 90.0001, 0, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), -90.0001, 0, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), 0, 180.0001, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), 0, -180.0001, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), 0, 0, 30000.0001),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mode_set_base_fixed(buf, sizeof(buf), 0, 0, -30000.0001),
            NEB_ERR_INVALID_PARAM);
}

// Manual §3.7, Table 3-10 -- every heading2 baseline token, plus invalid enum
static void test_mode_heading2(void) {
  BUILD_OK(
      neb_build_mode_set_heading2(buf, sizeof(buf), NEB_HEADING2_FIXLENGTH),
      "MODE HEADING2 FIXLENGTH");
  BUILD_OK(neb_build_mode_set_heading2(buf, sizeof(buf),
                                       NEB_HEADING2_VARIABLELENGTH),
           "MODE HEADING2 VARIABLELENGTH");
  BUILD_OK(neb_build_mode_set_heading2(buf, sizeof(buf), NEB_HEADING2_STATIC),
           "MODE HEADING2 STATIC");
  BUILD_OK(
      neb_build_mode_set_heading2(buf, sizeof(buf), NEB_HEADING2_LOWDYNAMIC),
      "MODE HEADING2 LOWDYNAMIC");
  BUILD_OK(neb_build_mode_set_heading2(buf, sizeof(buf), NEB_HEADING2_TRACTOR),
           "MODE HEADING2 TRACTOR");
  BUILD_ERR(
      neb_build_mode_set_heading2(buf, sizeof(buf), (neb_heading2_mode_t)999),
      NEB_ERR_INVALID_PARAM);
}

void run_mode_build_tests(void) {
  RUN_TEST(test_mode_query);
  RUN_TEST(test_mode_rover_default);
  RUN_TEST(test_mode_rover_profiles);
  RUN_TEST(test_mode_base_auto);
  RUN_TEST(test_mode_base_id);
  RUN_TEST(test_mode_base_self_optimize);
  RUN_TEST(test_mode_base_self_optimize_dist);
  RUN_TEST(test_mode_base_fixed);
  RUN_TEST(test_mode_heading2);
}
