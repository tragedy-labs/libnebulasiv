// test_hil_um982.c
//
// Hardware-in-the-loop verification against a real Holybro H-RTK Unicore UM982
// at $NEB_TEST_PORT (confirmed R4.10Build11826, 115200 baud). Run only via
// `make test-hardware` -- never part of `make test`.
//
// Purpose: exercise the UM982-only command builders that the unit suite can
// only check against the manual, and confirm the real UM982 ACCEPTS the exact
// wire strings we emit (an OK ack). This promotes the §4.8 heading /
// MODE HEADING2 / MASK CN0 / SBAS TIMEOUT paths from ALPHA to hardware-verified.
//
// Safety: every command here is RAM-only -- NO SAVECONFIG, NO FRESET. Changes
// revert on a power cycle, which restores Holybro's saved factory config.
// Values are chosen at or near the device defaults to stay minimally intrusive.
// ALGRESET is verified build-only (its live reset is intentionally NOT fired);
// its model/param gating is covered by the unit suite (test_algreset_gating).
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neb_config.h" // neb_build_config_algreset
#include "neb_core.h"
#include "neb_heading.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_protocol.h" // enums
#include "neb_rtk.h"

static neb_handle_t g_handle;

void setUp(void) {}
void tearDown(void) {}

// --- Read-only baselines --------------------------------------------------

// MODE query -- read-only (Manual §3.1).
static void test_hil_mode_query(void) {
  char resp[256];
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_mode_query(&g_handle, resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "response: OK"));
  TEST_ASSERT_NOT_NULL(strstr(resp, "#MODE,"));
}

// MASK query -- read-only (Manual §5.2).
static void test_hil_mask_query(void) {
  char resp[512];
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_mask_query(&g_handle, resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "response: OK"));
}

// --- §4.8 dual-antenna heading config (UM982-only) ------------------------
// Each asserts the device returned an OK ack (the wrapper returns NEB_OK).

static void test_hil_heading_reliability(void) {
  // Default threshold is 3 -- a no-op-ish value.
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_heading_reliability(&g_handle, 3));
}

// FIRMWARE DISCREPANCY (Manual §4.8 Table 4-13 vs Holybro R4.10Build11826): the
// manual says omitting both params uses the default configuration, but this
// firmware rejects bare "CONFIG HEADING LENGTH" with "PARSING FAILD GRAMMAR
// ERROR" -> NEB_ERR_NAK. We pin that observed behavior here; if a future
// firmware starts accepting it, this assertion flips and we revisit. The
// parameterized form (test_hil_heading_length) IS accepted.
static void test_hil_heading_length_default(void) {
  TEST_ASSERT_EQUAL_INT(NEB_ERR_NAK, neb_heading_length_default(&g_handle));
}

static void test_hil_heading_length(void) {
  // 100 cm baseline, 5 cm tolerance (RAM-only; power-cycle restores).
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_heading_length(&g_handle, 100, 5));
}

static void test_hil_heading_mode(void) {
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_heading_mode(&g_handle, NEB_HEADING2_FIXLENGTH));
}

// --- §4.9 heading/pitch offset (UM980/UM982) ------------------------------

static void test_hil_heading_offset(void) {
  // Zero correction -- valid and non-intrusive.
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_heading_offset(&g_handle, 0.0, 0.0));
}

// --- MASK CN0 / SBAS TIMEOUT (UM982 Build9669+; this unit is Build11826) ---

static void test_hil_mask_cn0(void) {
  // A mild 10 dB-Hz mask; the ack is what we verify, not the filtering effect.
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_mask_cn0(&g_handle, 10));
}

static void test_hil_sbas_timeout(void) {
  // Default 1200 s -- restores the documented default value.
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_rtk_sbas_timeout(&g_handle, 1200));
}

// --- MODE HEADING2 (§3.7) -- run last; it changes the operating mode -------

static void test_hil_mode_heading2(void) {
  TEST_ASSERT_EQUAL_INT(
      NEB_OK, neb_mode_set_heading2(&g_handle, NEB_HEADING2_FIXLENGTH));
}

// --- ALGRESET: build-only (live reset intentionally NOT fired) -------------

static void test_hil_algreset_build_only(void) {
  char buf[64];
  TEST_ASSERT_EQUAL_INT(
      NEB_OK, neb_build_config_algreset(buf, sizeof(buf), NEB_ALGRESET_RTK2));
  TEST_ASSERT_EQUAL_STRING("CONFIG ALGRESET RTK2", buf);
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_build_config_algreset(
                                    buf, sizeof(buf), NEB_ALGRESET_HEADING));
  TEST_ASSERT_EQUAL_STRING("CONFIG ALGRESET HEADING", buf);
}

int main(void) {
  const char *port = getenv("NEB_TEST_PORT");
  if (!port) {
    fprintf(stderr, "NEB_TEST_PORT not set (e.g. NEB_TEST_PORT=/dev/ttyUSB0)\n");
    return 2;
  }

  neb_status_t st = neb_open(&g_handle, NEB_MODEL_UM982, port, 115200);
  if (st != NEB_OK) {
    fprintf(stderr, "neb_open(%s): %s\n", port, neb_strerror(st));
    return 1;
  }

  UNITY_BEGIN();
  RUN_TEST(test_hil_mode_query);
  RUN_TEST(test_hil_mask_query);
  RUN_TEST(test_hil_heading_reliability);
  RUN_TEST(test_hil_heading_length_default);
  RUN_TEST(test_hil_heading_length);
  RUN_TEST(test_hil_heading_mode);
  RUN_TEST(test_hil_heading_offset);
  RUN_TEST(test_hil_mask_cn0);
  RUN_TEST(test_hil_sbas_timeout);
  RUN_TEST(test_hil_mode_heading2);
  RUN_TEST(test_hil_algreset_build_only);
  int rc = UNITY_END();

  neb_close(&g_handle);
  fprintf(stderr,
          "\nNOTE: RAM-only changes were sent (no SAVECONFIG). Power-cycle the "
          "UM982 to restore Holybro factory config.\n");
  return rc;
}
