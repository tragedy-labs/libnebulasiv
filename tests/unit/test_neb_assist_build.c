// test_neb_assist_build.c
//
// Manual-as-spec tests for the assisted position/time builders (Manual §6).
// Exact wire strings, documented formats, and parameter validation. Pure
// builder output -- no handle, no transport.
#include "unity.h"

#include "build_assert.h"
#include "neb_assist.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §6.1 -- $AIDPOS,Latitude,LatDir,Longitude,LonDir,Altitude
static void test_assist_position(void) {
  // Exact manual example.
  BUILD_OK(neb_build_assist_position(buf, sizeof(buf), 4002.229934,
                                     NEB_LAT_NORTH, 11618.096855, NEB_LON_EAST,
                                     37.254),
           "$AIDPOS,4002.229934,N,11618.096855,E,37.254");
  // Southern / western hemispheres.
  BUILD_OK(neb_build_assist_position(buf, sizeof(buf), 4002.229934,
                                     NEB_LAT_SOUTH, 11618.096855, NEB_LON_WEST,
                                     37.254),
           "$AIDPOS,4002.229934,S,11618.096855,W,37.254");
  // Below-ellipsoid altitude is legal; %.3f keeps fixed-point form (never
  // scientific notation) and rounds to the manual's 3 decimals.
  BUILD_OK(neb_build_assist_position(buf, sizeof(buf), 4002.229934,
                                     NEB_LAT_NORTH, 11618.096855, NEB_LON_EAST,
                                     -12.5),
           "$AIDPOS,4002.229934,N,11618.096855,E,-12.500");

  // Invalid hemisphere enums.
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 4002.0,
                                      (neb_lat_dir_t)999, 11618.0, NEB_LON_EAST,
                                      0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 4002.0, NEB_LAT_NORTH,
                                      11618.0, (neb_lon_dir_t)999, 0),
            NEB_ERR_INVALID_PARAM);

  // Minutes part >= 60 is invalid (40 deg 60.0 min).
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 4060.0, NEB_LAT_NORTH,
                                      11618.0, NEB_LON_EAST, 0),
            NEB_ERR_INVALID_PARAM);
  // Degrees out of range.
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 9100.0, NEB_LAT_NORTH,
                                      11618.0, NEB_LON_EAST, 0),
            NEB_ERR_INVALID_PARAM); // 91 deg latitude
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 4002.0, NEB_LAT_NORTH,
                                      18100.0, NEB_LON_EAST, 0),
            NEB_ERR_INVALID_PARAM); // 181 deg longitude
  // Negative magnitude (sign is carried by the hemisphere, not the value).
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), -4002.0, NEB_LAT_NORTH,
                                      11618.0, NEB_LON_EAST, 0),
            NEB_ERR_INVALID_PARAM);
  // Absurd altitude magnitude is rejected (finiteness / format-safety guard).
  BUILD_ERR(neb_build_assist_position(buf, sizeof(buf), 4002.229934,
                                      NEB_LAT_NORTH, 11618.096855, NEB_LON_EAST,
                                      1e6),
            NEB_ERR_INVALID_PARAM);
}

// Manual §6.2 -- $AIDTIME,Year,Month,Day,Hour,Minute,Second,Millisecond,Leapsec
static void test_assist_time(void) {
  // Exact manual example.
  BUILD_OK(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 3, 15, 2, 36, 400, 18),
      "$AIDTIME,2021,12,3,15,2,36,400,18");
  // Leap second (second == 60) is accepted.
  BUILD_OK(
      neb_build_assist_time(buf, sizeof(buf), 2016, 12, 31, 23, 59, 60, 0, 18),
      "$AIDTIME,2016,12,31,23,59,60,0,18");

  // Calendar-range rejections.
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 0, 3, 15, 2, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // month 0
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 13, 3, 15, 2, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // month 13
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 0, 15, 2, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // day 0
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 32, 15, 2, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // day 32
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 3, 24, 2, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // hour 24
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 3, 15, 60, 36, 400, 18),
      NEB_ERR_INVALID_PARAM); // minute 60
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 3, 15, 2, 61, 400, 18),
      NEB_ERR_INVALID_PARAM); // second 61
  BUILD_ERR(
      neb_build_assist_time(buf, sizeof(buf), 2021, 12, 3, 15, 2, 36, 1000, 18),
      NEB_ERR_INVALID_PARAM); // millisecond 1000
}

void run_assist_build_tests(void) {
  RUN_TEST(test_assist_position);
  RUN_TEST(test_assist_time);
}
