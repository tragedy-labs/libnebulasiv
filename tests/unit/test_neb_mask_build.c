// test_neb_mask_build.c
//
// Manual-as-spec tests for the MASK/UNMASK command builders (Manual §5). Exact
// wire strings, every constellation token, elevation and PRN bounds, and
// invalid-parameter rejection. Pure builder output -- no handle, no transport.
#include "unity.h"

#include <stdio.h>

#include "build_assert.h"
#include "neb_mask.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §5.1 -- MASK query
static void test_mask_query(void) {
  BUILD_OK(neb_build_mask_query(buf, sizeof(buf)), "MASK");
}

// Manual §5.2, Table 5-4 -- MASK <system>, every constellation
static void test_mask_gnss(void) {
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_GPS), "MASK GPS");
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_BDS), "MASK BDS");
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_GLO), "MASK GLO");
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_GAL), "MASK GAL");
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_QZSS), "MASK QZSS");
  BUILD_OK(neb_build_mask_gnss(buf, sizeof(buf), NEB_GNSS_IRNSS), "MASK IRNSS");
  BUILD_ERR(neb_build_mask_gnss(buf, sizeof(buf), (neb_gnss_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.3 -- UNMASK <system>
static void test_unmask_gnss(void) {
  BUILD_OK(neb_build_unmask_gnss(buf, sizeof(buf), NEB_GNSS_GPS), "UNMASK GPS");
  BUILD_OK(neb_build_unmask_gnss(buf, sizeof(buf), NEB_GNSS_GAL), "UNMASK GAL");
  BUILD_OK(neb_build_unmask_gnss(buf, sizeof(buf), NEB_GNSS_IRNSS),
           "UNMASK IRNSS");
  BUILD_ERR(neb_build_unmask_gnss(buf, sizeof(buf), (neb_gnss_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.2, Table 5-2 -- MASK <elevation>, range -90..90
static void test_mask_elevation(void) {
  BUILD_OK(neb_build_mask_elevation(buf, sizeof(buf), 10), "MASK 10");
  BUILD_OK(neb_build_mask_elevation(buf, sizeof(buf), 0), "MASK 0");
  BUILD_OK(neb_build_mask_elevation(buf, sizeof(buf), -90), "MASK -90"); // min
  BUILD_OK(neb_build_mask_elevation(buf, sizeof(buf), 90), "MASK 90");   // max
  BUILD_ERR(neb_build_mask_elevation(buf, sizeof(buf), -91),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mask_elevation(buf, sizeof(buf), 91),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.2, Table 5-2 -- MASK <elevation> <system>
static void test_mask_elevation_gnss(void) {
  BUILD_OK(neb_build_mask_elevation_gnss(buf, sizeof(buf), 10, NEB_GNSS_GPS),
           "MASK 10 GPS");
  BUILD_OK(neb_build_mask_elevation_gnss(buf, sizeof(buf), -90, NEB_GNSS_BDS),
           "MASK -90 BDS");
  BUILD_ERR(neb_build_mask_elevation_gnss(buf, sizeof(buf), 91, NEB_GNSS_GPS),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_mask_elevation_gnss(buf, sizeof(buf), 10, (neb_gnss_t)999),
      NEB_ERR_INVALID_PARAM);
}

// Manual §5.2, Table 5-3 -- MASK <system> PRN <id>
static void test_mask_satellite(void) {
  BUILD_OK(neb_build_mask_satellite(buf, sizeof(buf), NEB_GNSS_GPS, 10),
           "MASK GPS PRN 10"); // matches manual example
  BUILD_OK(neb_build_mask_satellite(buf, sizeof(buf), NEB_GNSS_QZSS, 194),
           "MASK QZSS PRN 194"); // high PRN, per the query example
  BUILD_ERR(neb_build_mask_satellite(buf, sizeof(buf), NEB_GNSS_GPS, 0),
            NEB_ERR_INVALID_PARAM); // PRN must be >= 1
  BUILD_ERR(neb_build_mask_satellite(buf, sizeof(buf), (neb_gnss_t)999, 10),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.3, Table 5-7 -- UNMASK <system> PRN <id>
static void test_unmask_satellite(void) {
  BUILD_OK(neb_build_unmask_satellite(buf, sizeof(buf), NEB_GNSS_GPS, 10),
           "UNMASK GPS PRN 10");
  BUILD_ERR(neb_build_unmask_satellite(buf, sizeof(buf), NEB_GNSS_GPS, 0),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.2, Table 5-4 -- MASK <frequency>: every documented frequency token.
static void test_mask_frequency(void) {
  static const struct {
    neb_gnss_freq_t freq;
    const char *token;
  } cases[] = {
      {NEB_FREQ_L1, "L1"},         {NEB_FREQ_L1CA, "L1CA"},
      {NEB_FREQ_L1C, "L1C"},       {NEB_FREQ_L2, "L2"},
      {NEB_FREQ_L2C, "L2C"},       {NEB_FREQ_L2P, "L2P"},
      {NEB_FREQ_L5, "L5"},         {NEB_FREQ_B1, "B1"},
      {NEB_FREQ_B2, "B2"},         {NEB_FREQ_B3, "B3"},
      {NEB_FREQ_B1I, "B1I"},       {NEB_FREQ_B2I, "B2I"},
      {NEB_FREQ_B3I, "B3I"},       {NEB_FREQ_BD3B1C, "BD3B1C"},
      {NEB_FREQ_BD3B2A, "BD3B2A"}, {NEB_FREQ_BD3B2B, "BD3B2B"},
      {NEB_FREQ_R1, "R1"},         {NEB_FREQ_R2, "R2"},
      {NEB_FREQ_R3, "R3"},         {NEB_FREQ_E1, "E1"},
      {NEB_FREQ_E5A, "E5a"},       {NEB_FREQ_E5B, "E5b"},
      {NEB_FREQ_E6C, "E6C"},       {NEB_FREQ_Q1, "Q1"},
      {NEB_FREQ_Q2, "Q2"},         {NEB_FREQ_Q5, "Q5"},
      {NEB_FREQ_Q1CA, "Q1CA"},     {NEB_FREQ_Q1C, "Q1C"},
      {NEB_FREQ_Q2C, "Q2C"},       {NEB_FREQ_I5, "I5"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    char buf[NEB_CMD_BUF_LEN], expected[NEB_CMD_BUF_LEN];
    TEST_ASSERT_EQUAL_INT(
        NEB_OK, neb_build_mask_frequency(buf, sizeof(buf), cases[i].freq));
    snprintf(expected, sizeof(expected), "MASK %s", cases[i].token);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
  }
  BUILD_ERR(neb_build_mask_frequency(buf, sizeof(buf), (neb_gnss_freq_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.3 -- UNMASK <frequency> (spot check; reuses the frequency table).
static void test_unmask_frequency(void) {
  BUILD_OK(neb_build_unmask_frequency(buf, sizeof(buf), NEB_FREQ_B1),
           "UNMASK B1");
  BUILD_OK(neb_build_unmask_frequency(buf, sizeof(buf), NEB_FREQ_E5A),
           "UNMASK E5a");
  BUILD_OK(neb_build_unmask_frequency(buf, sizeof(buf), NEB_FREQ_I5),
           "UNMASK I5");
  BUILD_ERR(neb_build_unmask_frequency(buf, sizeof(buf), (neb_gnss_freq_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.2 -- MASK <elevation> <frequency>
static void test_mask_elevation_frequency(void) {
  BUILD_OK(
      neb_build_mask_elevation_frequency(buf, sizeof(buf), 10, NEB_FREQ_B1),
      "MASK 10 B1");
  BUILD_OK(
      neb_build_mask_elevation_frequency(buf, sizeof(buf), -90, NEB_FREQ_E5A),
      "MASK -90 E5a");
  BUILD_ERR(
      neb_build_mask_elevation_frequency(buf, sizeof(buf), 91, NEB_FREQ_B1),
      NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_mask_elevation_frequency(buf, sizeof(buf), 10,
                                               (neb_gnss_freq_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §5.2, Table 5-5 -- RTCMCN0 / CN0, with and without a frequency.
static void test_mask_cn0(void) {
  BUILD_OK(neb_build_mask_rtcmcn0(buf, sizeof(buf), 35), "MASK RTCMCN0 35");
  BUILD_OK(neb_build_mask_rtcmcn0_frequency(buf, sizeof(buf), 35, NEB_FREQ_L1),
           "MASK RTCMCN0 35 L1");
  BUILD_OK(neb_build_mask_cn0(buf, sizeof(buf), 40), "MASK CN0 40");
  BUILD_OK(neb_build_mask_cn0_frequency(buf, sizeof(buf), 40, NEB_FREQ_B1I),
           "MASK CN0 40 B1I");
  BUILD_ERR(neb_build_mask_rtcmcn0_frequency(buf, sizeof(buf), 35,
                                             (neb_gnss_freq_t)999),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_mask_cn0_frequency(buf, sizeof(buf), 40, (neb_gnss_freq_t)999),
      NEB_ERR_INVALID_PARAM);
}

void run_mask_build_tests(void) {
  RUN_TEST(test_mask_query);
  RUN_TEST(test_mask_gnss);
  RUN_TEST(test_unmask_gnss);
  RUN_TEST(test_mask_elevation);
  RUN_TEST(test_mask_elevation_gnss);
  RUN_TEST(test_mask_satellite);
  RUN_TEST(test_unmask_satellite);
  RUN_TEST(test_mask_frequency);
  RUN_TEST(test_unmask_frequency);
  RUN_TEST(test_mask_elevation_frequency);
  RUN_TEST(test_mask_cn0);
}
