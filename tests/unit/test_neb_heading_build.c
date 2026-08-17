// test_neb_heading_build.c
//
// Manual-as-spec tests for the heading command builders (Manual §4.8-§4.9).
// Exact wire strings, documented bounds, invalid-parameter rejection. Pure
// builder output -- no handle, no transport.
//
// The wire strings are testable without hardware even though the §4.8 commands
// are UM982-only (alpha) and have not been run on a real device.
#include "unity.h"

#include "build_assert.h"
#include "neb_heading.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §4.8, Table 4-12 -- CONFIG HEADING <mode>, every baseline token
static void test_heading_mode(void) {
  BUILD_OK(neb_build_heading_mode(buf, sizeof(buf), NEB_HEADING2_FIXLENGTH),
           "CONFIG HEADING FIXLENGTH");
  BUILD_OK(
      neb_build_heading_mode(buf, sizeof(buf), NEB_HEADING2_VARIABLELENGTH),
      "CONFIG HEADING VARIABLELENGTH");
  BUILD_OK(neb_build_heading_mode(buf, sizeof(buf), NEB_HEADING2_STATIC),
           "CONFIG HEADING STATIC");
  BUILD_OK(neb_build_heading_mode(buf, sizeof(buf), NEB_HEADING2_LOWDYNAMIC),
           "CONFIG HEADING LOWDYNAMIC");
  BUILD_OK(neb_build_heading_mode(buf, sizeof(buf), NEB_HEADING2_TRACTOR),
           "CONFIG HEADING TRACTOR");
  BUILD_ERR(neb_build_heading_mode(buf, sizeof(buf), (neb_heading2_mode_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.8 -- CONFIG HEADING RELIABILITY <1..4>
static void test_heading_reliability(void) {
  BUILD_OK(neb_build_heading_reliability(buf, sizeof(buf), 3),
           "CONFIG HEADING RELIABILITY 3");
  BUILD_OK(neb_build_heading_reliability(buf, sizeof(buf), 1),
           "CONFIG HEADING RELIABILITY 1");
  BUILD_OK(neb_build_heading_reliability(buf, sizeof(buf), 4),
           "CONFIG HEADING RELIABILITY 4");
  BUILD_ERR(neb_build_heading_reliability(buf, sizeof(buf), 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_heading_reliability(buf, sizeof(buf), 5),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.8, Table 4-13 -- CONFIG HEADING LENGTH
static void test_heading_length(void) {
  BUILD_OK(neb_build_heading_length_default(buf, sizeof(buf)),
           "CONFIG HEADING LENGTH");
  BUILD_OK(neb_build_heading_length(buf, sizeof(buf), 20, 3),
           "CONFIG HEADING LENGTH 20 3"); // matches manual example values
  BUILD_ERR(neb_build_heading_length(buf, sizeof(buf), 0, 3),
            NEB_ERR_INVALID_PARAM); // length must be >= 1
}

// Manual §4.9, Table 4-14 -- CONFIG HEADING OFFSET <heading> <pitch>
static void test_heading_offset(void) {
  BUILD_OK(neb_build_heading_offset(buf, sizeof(buf), 90, 45),
           "CONFIG HEADING OFFSET 90 45"); // matches manual example
  BUILD_OK(neb_build_heading_offset(buf, sizeof(buf), -180, -90),
           "CONFIG HEADING OFFSET -180 -90"); // lower bounds
  BUILD_OK(neb_build_heading_offset(buf, sizeof(buf), 180, 90),
           "CONFIG HEADING OFFSET 180 90"); // upper bounds
  BUILD_ERR(neb_build_heading_offset(buf, sizeof(buf), 180.0001, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_heading_offset(buf, sizeof(buf), -180.0001, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_heading_offset(buf, sizeof(buf), 0, 90.0001),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_heading_offset(buf, sizeof(buf), 0, -90.0001),
            NEB_ERR_INVALID_PARAM);
}

void run_heading_build_tests(void) {
  RUN_TEST(test_heading_mode);
  RUN_TEST(test_heading_reliability);
  RUN_TEST(test_heading_length);
  RUN_TEST(test_heading_offset);
}
