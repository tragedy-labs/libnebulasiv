// test_neb_config_build.c
//
// Manual-as-spec tests for the CONFIG command builders (Manual §4). Exact wire
// strings, every enum token, and invalid-parameter rejection. Pure builder
// output -- no handle, no transport.
#include "unity.h"

#include "build_assert.h"
#include "neb_config.h"
#include "neb_protocol.h"
#include "tests.h"

// Manual §4.1 -- CONFIG query
static void test_config_query(void) {
  BUILD_OK(neb_build_config_query(buf, sizeof(buf)), "CONFIG");
}

// Manual §4.19, Table 4-24 -- PPP enable, every correction-source token
static void test_config_ppp_enable(void) {
  BUILD_OK(neb_build_config_ppp_enable(buf, sizeof(buf), NEB_PPP_B2B),
           "CONFIG PPP Enable B2b-PPP");
  BUILD_OK(neb_build_config_ppp_enable(buf, sizeof(buf), NEB_PPP_SSR_RX),
           "CONFIG PPP Enable SSR-RX");
  BUILD_ERR(neb_build_config_ppp_enable(buf, sizeof(buf), (neb_ppp_mode_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.19 -- PPP disable
static void test_config_ppp_disable(void) {
  BUILD_OK(neb_build_config_ppp_disable(buf, sizeof(buf)),
           "CONFIG PPP Disable");
}

// Manual §4.19, Table 4-25 -- PPP CONVERGE thresholds (must be > 0)
static void test_config_ppp_converge(void) {
  BUILD_OK(neb_build_config_ppp_converge(buf, sizeof(buf), 10.0, 20.0),
           "CONFIG PPP CONVERGE 10 20"); // matches manual example
  BUILD_OK(neb_build_config_ppp_converge(buf, sizeof(buf), 2.5, 3.5),
           "CONFIG PPP CONVERGE 2.5 3.5");
  // Non-positive thresholds are meaningless for a std-dev and are rejected.
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), 0.0, 20.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), 10.0, 0.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), -1.0, 20.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), 10.0, -5.0),
            NEB_ERR_INVALID_PARAM);
  // Absurd magnitudes are rejected before they reach %g's exponent range
  // (which would emit an unparseable "1e+06" on the wire).
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), 1e6, 20.0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_ppp_converge(buf, sizeof(buf), 10.0, 1e9),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.2, Table 4-4 -- serial port config; every port and every baud rate
static void test_config_serial(void) {
  // Each documented port.
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 115200),
           "CONFIG COM1 115200");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM2, 115200),
           "CONFIG COM2 115200");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM3, 115200),
           "CONFIG COM3 115200");

  // Every documented baud rate.
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 9600),
           "CONFIG COM1 9600");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 19200),
           "CONFIG COM1 19200");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 38400),
           "CONFIG COM1 38400");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 57600),
           "CONFIG COM1 57600");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 230400),
           "CONFIG COM1 230400");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 460800),
           "CONFIG COM1 460800");
  BUILD_OK(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 921600),
           "CONFIG COM1 921600");

  // Unsupported baud rate and invalid port are rejected.
  BUILD_ERR(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 12345),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_serial(buf, sizeof(buf), NEB_COM1, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(
      neb_build_config_serial(buf, sizeof(buf), (neb_com_port_t)999, 115200),
      NEB_ERR_INVALID_PARAM);
}

// Manual §4.14, Table 4-19 -- NMEA version; setter token is NMEA0183
static void test_config_nmea_version(void) {
  BUILD_OK(neb_build_config_nmea_version(buf, sizeof(buf), NEB_NMEA_V410),
           "CONFIG NMEA0183 V410");
  BUILD_OK(neb_build_config_nmea_version(buf, sizeof(buf), NEB_NMEA_V411),
           "CONFIG NMEA0183 V411");
  BUILD_ERR(
      neb_build_config_nmea_version(buf, sizeof(buf), (neb_nmea_version_t)999),
      NEB_ERR_INVALID_PARAM);
}

// Manual §4.20, Table 4-26 -- anti-jamming; every mode token
static void test_config_antijam(void) {
  BUILD_OK(neb_build_config_antijam(buf, sizeof(buf), NEB_ANTIJAM_DISABLE),
           "CONFIG ANTIJAM DISABLE");
  BUILD_OK(neb_build_config_antijam(buf, sizeof(buf), NEB_ANTIJAM_AUTO),
           "CONFIG ANTIJAM AUTO");
  BUILD_OK(neb_build_config_antijam(buf, sizeof(buf), NEB_ANTIJAM_FORCE),
           "CONFIG ANTIJAM FORCE");
  BUILD_ERR(neb_build_config_antijam(buf, sizeof(buf), (neb_antijam_mode_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.18, Table 4-23 -- AGNSS enable/disable
static void test_config_agnss(void) {
  BUILD_OK(neb_build_config_agnss_enable(buf, sizeof(buf)),
           "CONFIG AGNSS Enable");
  BUILD_OK(neb_build_config_agnss_disable(buf, sizeof(buf)),
           "CONFIG AGNSS Disable");
}

// Manual §4.4, Table 4-7 -- undulation: auto, plus value with documented range
static void test_config_undulation(void) {
  BUILD_OK(neb_build_config_undulation_auto(buf, sizeof(buf)),
           "CONFIG UNDULATION Auto");
  BUILD_OK(neb_build_config_undulation(buf, sizeof(buf), 9.7),
           "CONFIG UNDULATION 9.7000"); // four decimals per the manual
  BUILD_OK(neb_build_config_undulation(buf, sizeof(buf), 0.0),
           "CONFIG UNDULATION 0.0000");
  BUILD_OK(neb_build_config_undulation(buf, sizeof(buf), -1000.0),
           "CONFIG UNDULATION -1000.0000"); // lower bound
  BUILD_OK(neb_build_config_undulation(buf, sizeof(buf), 1000.0),
           "CONFIG UNDULATION 1000.0000"); // upper bound
  BUILD_ERR(neb_build_config_undulation(buf, sizeof(buf), -1000.0001),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_undulation(buf, sizeof(buf), 1000.0001),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.12, Table 4-17 -- smooth engines; epoch range 0..100; PSRVEL lower
// case enable/disable per the manual example
static void test_config_smooth(void) {
  BUILD_OK(neb_build_config_smooth_rtkheight(buf, sizeof(buf), 10),
           "CONFIG SMOOTH RTKHEIGHT 10");
  BUILD_OK(neb_build_config_smooth_rtkheight(buf, sizeof(buf), 0),
           "CONFIG SMOOTH RTKHEIGHT 0");
  BUILD_OK(neb_build_config_smooth_rtkheight(buf, sizeof(buf), 100),
           "CONFIG SMOOTH RTKHEIGHT 100");
  BUILD_ERR(neb_build_config_smooth_rtkheight(buf, sizeof(buf), 101),
            NEB_ERR_INVALID_PARAM);

  BUILD_OK(neb_build_config_smooth_heading(buf, sizeof(buf), 10),
           "CONFIG SMOOTH HEADING 10");
  BUILD_OK(neb_build_config_smooth_heading(buf, sizeof(buf), 100),
           "CONFIG SMOOTH HEADING 100");
  BUILD_ERR(neb_build_config_smooth_heading(buf, sizeof(buf), 101),
            NEB_ERR_INVALID_PARAM);

  BUILD_OK(neb_build_config_smooth_psrvel_enable(buf, sizeof(buf)),
           "CONFIG SMOOTH PSRVEL enable");
  BUILD_OK(neb_build_config_smooth_psrvel_disable(buf, sizeof(buf)),
           "CONFIG SMOOTH PSRVEL disable");
}

// Manual §4.15, Table 4-20 -- RTCM B1C B2a enable/disable (exact token casing)
static void test_config_rtcm_b1c_b2a(void) {
  BUILD_OK(neb_build_config_rtcm_b1c_b2a_enable(buf, sizeof(buf)),
           "CONFIG RTCMB1CB2a Enable");
  BUILD_OK(neb_build_config_rtcm_b1c_b2a_disable(buf, sizeof(buf)),
           "CONFIG RTCMB1CB2a Disable");
}

// Manual §4.25, Table 4-33 -- ionospheric model; every documented token
static void test_config_ionmode(void) {
  BUILD_OK(neb_build_config_ionmode(buf, sizeof(buf), NEB_IONMODE_GPSK8),
           "CONFIG IONMODE GPSK8");
  BUILD_OK(neb_build_config_ionmode(buf, sizeof(buf), NEB_IONMODE_BD2K8),
           "CONFIG IONMODE BD2K8");
  BUILD_OK(neb_build_config_ionmode(buf, sizeof(buf), NEB_IONMODE_BD3GIM),
           "CONFIG IONMODE BD3GIM");
  BUILD_OK(neb_build_config_ionmode(buf, sizeof(buf), NEB_IONMODE_GALNTCM),
           "CONFIG IONMODE GALNTCM");
  BUILD_ERR(neb_build_config_ionmode(buf, sizeof(buf), (neb_ionmode_t)999),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.13, Table 4-18 -- multi-path mitigation (uppercase ENABLE/DISABLE)
static void test_config_mmp(void) {
  BUILD_OK(neb_build_config_mmp_enable(buf, sizeof(buf)), "CONFIG MMP ENABLE");
  BUILD_OK(neb_build_config_mmp_disable(buf, sizeof(buf)),
           "CONFIG MMP DISABLE");
}

// Manual §4.11, Table 4-16 -- EVENT; polarity tokens and TGUARD
// range 2..3599999
static void test_config_event(void) {
  BUILD_OK(neb_build_config_event_disable(buf, sizeof(buf)),
           "CONFIG EVENT DISABLE");
  BUILD_OK(neb_build_config_event_enable(buf, sizeof(buf),
                                         NEB_POLARITY_POSITIVE, 10),
           "CONFIG EVENT ENABLE POSITIVE 10"); // matches manual example
  BUILD_OK(
      neb_build_config_event_enable(buf, sizeof(buf), NEB_POLARITY_NEGATIVE, 4),
      "CONFIG EVENT ENABLE NEGATIVE 4"); // default TGUARD
  BUILD_OK(
      neb_build_config_event_enable(buf, sizeof(buf), NEB_POLARITY_POSITIVE, 2),
      "CONFIG EVENT ENABLE POSITIVE 2"); // min TGUARD
  BUILD_OK(neb_build_config_event_enable(buf, sizeof(buf),
                                         NEB_POLARITY_POSITIVE, 3599999),
           "CONFIG EVENT ENABLE POSITIVE 3599999"); // max TGUARD
  BUILD_ERR(
      neb_build_config_event_enable(buf, sizeof(buf), NEB_POLARITY_POSITIVE, 1),
      NEB_ERR_INVALID_PARAM); // below min
  BUILD_ERR(neb_build_config_event_enable(buf, sizeof(buf),
                                          NEB_POLARITY_POSITIVE, 3600000),
            NEB_ERR_INVALID_PARAM); // above max
  BUILD_ERR(
      neb_build_config_event_enable(buf, sizeof(buf), (neb_polarity_t)999, 10),
      NEB_ERR_INVALID_PARAM); // bad polarity
}

// Manual §4.16, Table 4-21 -- RTCM phaserange rate sign
static void test_config_rtcmphaserate(void) {
  BUILD_OK(neb_build_config_rtcmphaserate_positive(buf, sizeof(buf)),
           "CONFIG RTCMPHASERATE POSITIVE");
  BUILD_OK(neb_build_config_rtcmphaserate_negative(buf, sizeof(buf)),
           "CONFIG RTCMPHASERATE NEGATIVE");
}

// Manual §4.17, Table 4-22 -- Doppler position prediction (uppercase tokens)
static void test_config_psrveldrpos(void) {
  BUILD_OK(neb_build_config_psrveldrpos_enable(buf, sizeof(buf)),
           "CONFIG PSRVELDRPOS ENABLE");
  BUILD_OK(neb_build_config_psrveldrpos_disable(buf, sizeof(buf)),
           "CONFIG PSRVELDRPOS DISABLE");
}

// Manual §4.3, Tables 4-5/4-6 -- PPS: modes, timerefs, polarity, and the
// documented numeric bounds.
static void test_config_pps(void) {
  BUILD_OK(neb_build_config_pps_disable(buf, sizeof(buf)),
           "CONFIG PPS DISABLE");

  // Exact manual example.
  BUILD_OK(neb_build_config_pps_enable(
               buf, sizeof(buf), NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
               NEB_POLARITY_POSITIVE, 500000, 1000, 0, 0),
           "CONFIG PPS ENABLE GPS POSITIVE 500000 1000 0 0");

  // Every mode token.
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE2,
                                       NEB_PPS_TIMEREF_GPS,
                                       NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
           "CONFIG PPS ENABLE2 GPS POSITIVE 100 1000 0 0");
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE3,
                                       NEB_PPS_TIMEREF_GPS,
                                       NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
           "CONFIG PPS ENABLE3 GPS POSITIVE 100 1000 0 0");

  // Every time reference token.
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                       NEB_PPS_TIMEREF_BDS,
                                       NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
           "CONFIG PPS ENABLE BDS POSITIVE 100 1000 0 0");
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                       NEB_PPS_TIMEREF_GAL,
                                       NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
           "CONFIG PPS ENABLE GAL POSITIVE 100 1000 0 0");
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                       NEB_PPS_TIMEREF_GLO,
                                       NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
           "CONFIG PPS ENABLE GLO POSITIVE 100 1000 0 0");

  // Polarity NEGATIVE, plus boundary period (50) and delays (-32768/32767).
  BUILD_OK(neb_build_config_pps_enable(
               buf, sizeof(buf), NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
               NEB_POLARITY_NEGATIVE, 100, 50, -32768, 32767),
           "CONFIG PPS ENABLE GPS NEGATIVE 100 50 -32768 32767");
  // Upper period bound.
  BUILD_OK(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                       NEB_PPS_TIMEREF_GPS,
                                       NEB_POLARITY_POSITIVE, 100, 20000, 0, 0),
           "CONFIG PPS ENABLE GPS POSITIVE 100 20000 0 0");

  // Period out of range.
  BUILD_ERR(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                        NEB_PPS_TIMEREF_GPS,
                                        NEB_POLARITY_POSITIVE, 100, 49, 0, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_pps_enable(
                buf, sizeof(buf), NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
                NEB_POLARITY_POSITIVE, 100, 20001, 0, 0),
            NEB_ERR_INVALID_PARAM);
  // Width not smaller than the period (period 50 ms == 50000 us).
  BUILD_ERR(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                        NEB_PPS_TIMEREF_GPS,
                                        NEB_POLARITY_POSITIVE, 50000, 50, 0, 0),
            NEB_ERR_INVALID_PARAM);
  // Delay bounds.
  BUILD_ERR(neb_build_config_pps_enable(
                buf, sizeof(buf), NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
                NEB_POLARITY_POSITIVE, 100, 1000, -32769, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_pps_enable(
                buf, sizeof(buf), NEB_PPS_ENABLE, NEB_PPS_TIMEREF_GPS,
                NEB_POLARITY_POSITIVE, 100, 1000, 0, 32768),
            NEB_ERR_INVALID_PARAM);
  // Bad enums.
  BUILD_ERR(neb_build_config_pps_enable(buf, sizeof(buf), (neb_pps_mode_t)999,
                                        NEB_PPS_TIMEREF_GPS,
                                        NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                        (neb_pps_timeref_t)999,
                                        NEB_POLARITY_POSITIVE, 100, 1000, 0, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_pps_enable(buf, sizeof(buf), NEB_PPS_ENABLE,
                                        NEB_PPS_TIMEREF_GPS,
                                        (neb_polarity_t)999, 100, 1000, 0, 0),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.21, Table 4-28 -- signal group; TypeNum 0..7, master and dual forms
static void test_config_signalgroup(void) {
  BUILD_OK(neb_build_config_signalgroup(buf, sizeof(buf), 1),
           "CONFIG SIGNALGROUP 1"); // matches manual example
  BUILD_OK(neb_build_config_signalgroup(buf, sizeof(buf), 0),
           "CONFIG SIGNALGROUP 0");
  BUILD_OK(neb_build_config_signalgroup(buf, sizeof(buf), 7),
           "CONFIG SIGNALGROUP 7");
  BUILD_ERR(neb_build_config_signalgroup(buf, sizeof(buf), 8),
            NEB_ERR_INVALID_PARAM);

  BUILD_OK(neb_build_config_signalgroup_dual(buf, sizeof(buf), 2, 3),
           "CONFIG SIGNALGROUP 2 3"); // matches manual example
  BUILD_OK(neb_build_config_signalgroup_dual(buf, sizeof(buf), 7, 7),
           "CONFIG SIGNALGROUP 7 7");
  BUILD_ERR(neb_build_config_signalgroup_dual(buf, sizeof(buf), 8, 0),
            NEB_ERR_INVALID_PARAM);
  BUILD_ERR(neb_build_config_signalgroup_dual(buf, sizeof(buf), 0, 8),
            NEB_ERR_INVALID_PARAM);
}

// Manual §4.24, Table 4-32 -- algorithm reset; every type token
static void test_config_algreset(void) {
  BUILD_OK(neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_RTK1),
           "CONFIG ALGRESET RTK1");
  BUILD_OK(neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_RTK2),
           "CONFIG ALGRESET RTK2");
  BUILD_OK(neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_HEADING),
           "CONFIG ALGRESET HEADING");
  BUILD_OK(neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_PPP),
           "CONFIG ALGRESET PPP");
  BUILD_OK(neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_ADR),
           "CONFIG ALGRESET ADR");
  BUILD_ERR(
      neb_build_config_algreset(buf, sizeof(buf), (neb_algreset_type_t)99),
      NEB_ERR_INVALID_PARAM);
}

void run_config_build_tests(void) {
  RUN_TEST(test_config_query);
  RUN_TEST(test_config_ppp_enable);
  RUN_TEST(test_config_ppp_disable);
  RUN_TEST(test_config_ppp_converge);
  RUN_TEST(test_config_serial);
  RUN_TEST(test_config_nmea_version);
  RUN_TEST(test_config_antijam);
  RUN_TEST(test_config_agnss);
  RUN_TEST(test_config_undulation);
  RUN_TEST(test_config_smooth);
  RUN_TEST(test_config_rtcm_b1c_b2a);
  RUN_TEST(test_config_ionmode);
  RUN_TEST(test_config_mmp);
  RUN_TEST(test_config_event);
  RUN_TEST(test_config_rtcmphaserate);
  RUN_TEST(test_config_psrveldrpos);
  RUN_TEST(test_config_pps);
  RUN_TEST(test_config_signalgroup);
  RUN_TEST(test_config_algreset);
}
