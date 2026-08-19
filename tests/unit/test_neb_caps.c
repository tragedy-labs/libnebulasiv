// test_neb_caps.c
//
// Capability-gating tests. Two layers:
//  1. neb_caps_for_model() returns exactly the capability set the manual's
//     "Applicable to:" lines document for each model.
//  2. Every model-restricted command proceeds on a supported model (reaching
//     the transport) and returns NEB_ERR_UNSUPPORTED on an unsupported model
//     without any transport call.
#include "unity.h"

#include "mock_transport.h"
#include "neb_admin.h"
#include "neb_assist.h"
#include "neb_config.h"
#include "neb_core.h"
#include "neb_heading.h"
#include "neb_logging.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_protocol.h"
#include "neb_rtk.h"
#include "tests.h"

// A generic accepted ack; the parser ignores the ack checksum.
#define OK_ACK "$command,x,response: OK*26\r\n"

// Run `callexpr` (which references handle `h`) on `model` and assert the status
// and whether the transport was touched (1 = a write happened, 0 = none).
#define EXPECT_GATED(model, expected_status, expected_wrote, callexpr)         \
  do {                                                                         \
    mock_transport_t mock;                                                     \
    mock_transport_init(&mock);                                                \
    mock_transport_set_response_str(&mock, OK_ACK);                            \
    neb_handle_t h = mock_handle(&mock, (model));                              \
    TEST_ASSERT_EQUAL_INT((expected_status), (callexpr));                      \
    TEST_ASSERT_EQUAL_INT((expected_wrote), mock.write_calls > 0 ? 1 : 0);     \
  } while (0)

// Manual §3/§4 "Applicable to:" lines, encoded as exact capability sets.
static void test_caps_bitfields(void) {
  TEST_ASSERT_EQUAL_HEX32(
      NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_ANTIJAM | NEB_CAP_ROVER_PROFILE |
          NEB_CAP_HEADING2_MODE | NEB_CAP_CONFIG_AGNSS | NEB_CAP_IONMODE |
          NEB_CAP_EVENT | NEB_CAP_PPS_ENABLE23 | NEB_CAP_STANDALONE |
          NEB_CAP_SBAS | NEB_CAP_MASK | NEB_CAP_ALGRESET | NEB_CAP_ADMIN |
          NEB_CAP_LOGGING,
      neb_caps_for_model(NEB_MODEL_UM960));
  TEST_ASSERT_EQUAL_HEX32(NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_MASK |
                              NEB_CAP_ADMIN | NEB_CAP_LOGGING,
                          neb_caps_for_model(NEB_MODEL_UM960L));
  TEST_ASSERT_EQUAL_HEX32(
      NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_PPP | NEB_CAP_AGNSS |
          NEB_CAP_ANTIJAM | NEB_CAP_ROVER_PROFILE | NEB_CAP_HEADING2_MODE |
          NEB_CAP_CONFIG_AGNSS | NEB_CAP_RTCM_B1C_B2A | NEB_CAP_IONMODE |
          NEB_CAP_MMP | NEB_CAP_EVENT | NEB_CAP_RTCMPHASERATE |
          NEB_CAP_PSRVELDRPOS | NEB_CAP_PPS_ENABLE23 | NEB_CAP_STANDALONE |
          NEB_CAP_SBAS | NEB_CAP_MASK | NEB_CAP_HEADING_OFFSET |
          NEB_CAP_SIGNALGROUP | NEB_CAP_ALGRESET | NEB_CAP_ADMIN |
          NEB_CAP_LOGGING,
      neb_caps_for_model(NEB_MODEL_UM980));
  TEST_ASSERT_EQUAL_HEX32(
      NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_PPP | NEB_CAP_HEADING |
          NEB_CAP_AGNSS | NEB_CAP_ROVER_PROFILE | NEB_CAP_HEADING2_MODE |
          NEB_CAP_CONFIG_AGNSS | NEB_CAP_RTCM_B1C_B2A | NEB_CAP_IONMODE |
          NEB_CAP_EVENT | NEB_CAP_RTCMPHASERATE | NEB_CAP_PPS_ENABLE23 |
          NEB_CAP_STANDALONE | NEB_CAP_SBAS | NEB_CAP_SBAS_TIMEOUT |
          NEB_CAP_MASK | NEB_CAP_MASK_CN0 | NEB_CAP_HEADING_OFFSET |
          NEB_CAP_SIGNALGROUP | NEB_CAP_ALGRESET | NEB_CAP_ADMIN |
          NEB_CAP_LOGGING,
      neb_caps_for_model(NEB_MODEL_UM982));
}

// Manual §4.19 -- PPP is UM980/UM982 only. Test all three PPP commands on both
// supported models and both unsupported models.
static void test_ppp_gating(void) {
  // Supported: reaches the transport, returns OK.
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_ppp_enable(&h, NEB_PPP_B2B));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_ppp_enable(&h, NEB_PPP_B2B));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_ppp_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_ppp_converge(&h, 10.0, 20.0));

  // Unsupported: rejected before any write.
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_ppp_enable(&h, NEB_PPP_B2B));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_ppp_enable(&h, NEB_PPP_B2B));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_ppp_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_ppp_converge(&h, 10.0, 20.0));
}

// Manual §3.6 -- rover profiles are UM960/UM980/UM982; not UM960L.
static void test_rover_profile_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_mode_set_rover_profile(&h, NEB_ROVER_SURVEY));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_mode_set_rover_profile(&h, NEB_ROVER_SURVEY));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_mode_set_rover_profile(&h, NEB_ROVER_UAV));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_mode_set_rover_profile(&h, NEB_ROVER_SURVEY));
}

// Manual §3.7 -- heading2 mode is UM960/UM980/UM982; not UM960L.
static void test_heading2_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_mode_set_heading2(&h, NEB_HEADING2_FIXLENGTH));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_mode_set_heading2(&h, NEB_HEADING2_FIXLENGTH));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_mode_set_heading2(&h, NEB_HEADING2_STATIC));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_mode_set_heading2(&h, NEB_HEADING2_FIXLENGTH));
}

// Manual §4.20 -- ANTIJAM is UM960/UM980 only.
static void test_antijam_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_config_antijam(&h, NEB_ANTIJAM_AUTO));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_antijam(&h, NEB_ANTIJAM_FORCE));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_ERR_UNSUPPORTED, 0,
               neb_config_antijam(&h, NEB_ANTIJAM_AUTO));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_antijam(&h, NEB_ANTIJAM_AUTO));
}

// Manual §4.18 -- CONFIG AGNSS is UM960/UM980/UM982; not UM960L.
static void test_config_agnss_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1, neb_config_agnss_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_agnss_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_config_agnss_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_agnss_enable(&h));
}

// Manual §4.15 -- RTCM B1C B2a is UM980/UM982 only.
static void test_rtcm_b1c_b2a_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_rtcm_b1c_b2a_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_config_rtcm_b1c_b2a_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_rtcm_b1c_b2a_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_rtcm_b1c_b2a_enable(&h));
}

// Manual §4.25 -- IONMODE is UM960/UM980/UM982; not UM960L.
static void test_ionmode_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_config_ionmode(&h, NEB_IONMODE_GPSK8));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_ionmode(&h, NEB_IONMODE_GPSK8));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_ionmode(&h, NEB_IONMODE_GALNTCM));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_ionmode(&h, NEB_IONMODE_GPSK8));
}

// Manual §4.13 -- MMP is UM980 only.
static void test_mmp_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_mmp_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_mmp_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_ERR_UNSUPPORTED, 0,
               neb_config_mmp_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_mmp_disable(&h));
}

// Manual §4.11 -- EVENT is UM960/UM980/UM982; not UM960L.
static void test_event_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1, neb_config_event_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_event_enable(&h, NEB_POLARITY_POSITIVE, 10));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_config_event_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_event_disable(&h));
}

// Manual §4.16 -- RTCMPHASERATE is UM980/UM982 only.
static void test_rtcmphaserate_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_rtcmphaserate_positive(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_rtcmphaserate_negative(&h));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_rtcmphaserate_positive(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_rtcmphaserate_positive(&h));
}

// Manual §4.17 -- PSRVELDRPOS is UM980 only.
static void test_psrveldrpos_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_psrveldrpos_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_psrveldrpos_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_ERR_UNSUPPORTED, 0,
               neb_config_psrveldrpos_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_psrveldrpos_disable(&h));
}

// Manual §4.3 -- PPS is available on all models, but ENABLE2/ENABLE3 are not
// supported on the UM960L (a per-value model rule enforced in the wrapper).
static void test_pps_gating(void) {
  // Base PPS (disable, and ENABLE) works on every model incl. UM960L.
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_OK, 1, neb_config_pps_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_OK, 1,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
  // ENABLE2/ENABLE3 rejected on UM960L before any write.
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE2, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE3, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
  // ENABLE2 accepted on the other three models.
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE2, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE2, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_pps_enable(&h, NEB_PPS_ENABLE3, NEB_PPS_TIMEREF_GPS,
                                     NEB_POLARITY_POSITIVE, 100, 1000, 0, 0));
}

// Manual §4.7 -- STANDALONE is UM960/UM980/UM982; not UM960L.
static void test_standalone_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1, neb_rtk_standalone_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_rtk_standalone_enable_time(&h, 60));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_rtk_standalone_enable(&h));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_rtk_standalone_disable(&h));
}

// Manual §4.10 -- SBAS enable/disable is UM960/UM980/UM982; not UM960L.
static void test_sbas_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_rtk_sbas_enable(&h, NEB_SBAS_AUTO));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_rtk_sbas_disable(&h));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_rtk_sbas_enable(&h, NEB_SBAS_WAAS));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_rtk_sbas_enable(&h, NEB_SBAS_AUTO));
}

// Manual §4.10 fn -- SBAS TIMEOUT is UM982 only.
static void test_sbas_timeout_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_rtk_sbas_timeout(&h, 600));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_rtk_sbas_timeout(&h, 600));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_ERR_UNSUPPORTED, 0,
               neb_rtk_sbas_timeout(&h, 600));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_rtk_sbas_timeout(&h, 600));
}

// Manual §5.2 fn -- MASK RTCMCN0/CN0 is UM982 only.
static void test_mask_cn0_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_mask_rtcmcn0(&h, 35));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_mask_cn0(&h, 40));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_mask_rtcmcn0(&h, 35));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_ERR_UNSUPPORTED, 0, neb_mask_cn0(&h, 40));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_mask_rtcmcn0(&h, 35));
}

// Manual §6 -- assisted position/time (AGNSS input) is UM980/UM982 only.
static void test_assist_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_assist_time(&h, 2021, 12, 3, 15, 2, 36, 400, 18));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_assist_position(&h, 4002.229934, NEB_LAT_NORTH, 11618.096855,
                                   NEB_LON_EAST, 37.254));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_assist_time(&h, 2021, 12, 3, 15, 2, 36, 400, 18));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_assist_time(&h, 2021, 12, 3, 15, 2, 36, 400, 18));
}

// Manual §4.8 -- CONFIG HEADING (mode/reliability/length) is UM982 only.
// Manual §4.9 -- CONFIG HEADING OFFSET is UM980/UM982.
static void test_heading_gating(void) {
  // §4.8 mode: UM982 only.
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_heading_mode(&h, NEB_HEADING2_FIXLENGTH));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_ERR_UNSUPPORTED, 0,
               neb_heading_mode(&h, NEB_HEADING2_FIXLENGTH));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_heading_reliability(&h, 3));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_heading_length_default(&h));
  // §4.9 offset: UM980 and UM982.
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_heading_offset(&h, 90, 45));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_heading_offset(&h, 90, 45));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_heading_offset(&h, 90, 45));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_heading_offset(&h, 90, 45));
}

// Manual §4.21 -- SIGNALGROUP is UM980/UM982.
static void test_signalgroup_gating(void) {
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_config_signalgroup(&h, 1));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_signalgroup_dual(&h, 2, 3));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_signalgroup(&h, 1));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_signalgroup(&h, 1));
}

// Manual §4.24, Table 4-32 -- ALGRESET base is UM960/UM980/UM982; RTK2/HEADING
// need UM982, PPP needs UM980/UM982 (per-value gating).
static void test_algreset_gating(void) {
  // RTK1/ADR available on all three ALGRESET-capable models.
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1,
               neb_config_algreset(&h, NEB_ALGRESET_RTK1));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_algreset(&h, NEB_ALGRESET_ADR));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_ERR_UNSUPPORTED, 0,
               neb_config_algreset(&h, NEB_ALGRESET_RTK1)); // no ALGRESET cap
  // RTK2 / HEADING: UM982 only.
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1,
               neb_config_algreset(&h, NEB_ALGRESET_RTK2));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_ERR_UNSUPPORTED, 0,
               neb_config_algreset(&h, NEB_ALGRESET_RTK2));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_algreset(&h, NEB_ALGRESET_HEADING));
  // PPP: UM980/UM982.
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_config_algreset(&h, NEB_ALGRESET_PPP));
  EXPECT_GATED(NEB_MODEL_UM960, NEB_ERR_UNSUPPORTED, 0,
               neb_config_algreset(&h, NEB_ALGRESET_PPP));
}

// Commands available on all models still work on every model (spot check the
// query, which every model supports).
static void test_common_commands_all_models(void) {
  EXPECT_GATED(NEB_MODEL_UM960, NEB_OK, 1, neb_mode_query(&h, NULL, 0));
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_OK, 1, neb_config_query(&h, NULL, 0));
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1, neb_mode_query(&h, NULL, 0));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_config_query(&h, NULL, 0));
  // MASK is a top-level command on every model, including the UM960L.
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_OK, 1, neb_mask_gnss(&h, NEB_GNSS_GPS));
  EXPECT_GATED(NEB_MODEL_UM982, NEB_OK, 1, neb_mask_query(&h, NULL, 0));
  // UNLOG is a top-level admin command on every model.
  EXPECT_GATED(NEB_MODEL_UM960L, NEB_OK, 1, neb_admin_unlog_all(&h));
  // Data-output (logging) is available on every model.
  EXPECT_GATED(NEB_MODEL_UM980, NEB_OK, 1,
               neb_logging_periodic(&h, "GPGGA", 1.0));
}

void run_caps_tests(void) {
  RUN_TEST(test_caps_bitfields);
  RUN_TEST(test_ppp_gating);
  RUN_TEST(test_rover_profile_gating);
  RUN_TEST(test_heading2_gating);
  RUN_TEST(test_antijam_gating);
  RUN_TEST(test_config_agnss_gating);
  RUN_TEST(test_rtcm_b1c_b2a_gating);
  RUN_TEST(test_ionmode_gating);
  RUN_TEST(test_mmp_gating);
  RUN_TEST(test_event_gating);
  RUN_TEST(test_rtcmphaserate_gating);
  RUN_TEST(test_psrveldrpos_gating);
  RUN_TEST(test_pps_gating);
  RUN_TEST(test_standalone_gating);
  RUN_TEST(test_sbas_gating);
  RUN_TEST(test_sbas_timeout_gating);
  RUN_TEST(test_mask_cn0_gating);
  RUN_TEST(test_assist_gating);
  RUN_TEST(test_heading_gating);
  RUN_TEST(test_signalgroup_gating);
  RUN_TEST(test_algreset_gating);
  RUN_TEST(test_common_commands_all_models);
}
