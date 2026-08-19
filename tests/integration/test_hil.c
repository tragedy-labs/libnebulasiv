// test_hil.c
//
// Hardware-in-the-loop verification against whatever N4 receiver is attached.
// Run only via `make test-hardware` -- never part of `make test`.
//
// One suite, every device. There is no per-model test file: the run identifies
// the receiver with VERSIONA and then decides, per test, what that particular
// model and firmware build should do. A test therefore has three meaningful
// outcomes rather than two:
//
//   - the model has the command and the build is new enough -> expect the
//     device to accept it (or, for a pinned discrepancy, to reject it);
//   - the model does not have the command -> expect the capability gate to
//     refuse it before anything reaches the wire. This is a real assertion,
//     not a skip: it confirms neb_caps_for_model() against actual silicon;
//   - the model has it but the firmware build is below the documented minimum
//     -> skipped, because the device is the only arbiter and we cannot tell an
//     old build apart from a broken one.
//
// Safety: every command sent here is RAM-only -- NO SAVECONFIG, NO FRESET.
// Changes revert on a power cycle. Values are chosen at or near the device
// defaults to stay minimally intrusive. Set NEB_TEST_LEVEL=read to run only
// the read-only queries and pure builders against an in-service receiver.
//
// Environment:
//   NEB_TEST_PORT         required -- the port; never guessed
//   NEB_TEST_BAUD         default 115200
//   NEB_TEST_BOARD        integrator/board; the one field no query can reveal
//   NEB_TEST_LEVEL        read | ram (default ram)
//   NEB_TEST_CONTRIBUTOR  who ran it, for the generated matrix
//   NEB_TEST_RESULTS_DIR  default tests/results
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hil_device.h"

#include "neb_config.h" // neb_build_config_algreset
#include "neb_core.h"
#include "neb_heading.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_protocol.h"
#include "neb_rtk.h"

static neb_handle_t g_handle;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Test actions
// ---------------------------------------------------------------------------
// An action returns the status the device gave, and the harness decides what
// that should have been. A custom test asserts for itself -- used where the
// check is richer than a status code, or where nothing touches the wire.

static void custom_mode_query(void) {
  char response[256];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_mode_query(&g_handle, response, sizeof(response)));
  TEST_ASSERT_NOT_NULL(strstr(response, "response: OK"));
  TEST_ASSERT_NOT_NULL(strstr(response, "#MODE,"));
}

static void custom_config_query(void) {
  char response[1024];
  TEST_ASSERT_EQUAL_INT(
      NEB_OK, neb_config_query(&g_handle, response, sizeof(response)));
  TEST_ASSERT_NOT_NULL(strstr(response, "response: OK"));
}

static void custom_mask_query(void) {
  char response[512];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_mask_query(&g_handle, response, sizeof(response)));
  TEST_ASSERT_NOT_NULL(strstr(response, "response: OK"));
}

// ALGRESET is verified build-only: its wire string is checked, its live reset
// is intentionally never fired. Model/parameter gating is covered by the unit
// suite (test_algreset_gating).
static void custom_algreset_build_only(void) {
  char buffer[64];
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_build_config_algreset(
                                    buffer, sizeof(buffer), NEB_ALGRESET_RTK2));
  TEST_ASSERT_EQUAL_STRING("CONFIG ALGRESET RTK2", buffer);
  TEST_ASSERT_EQUAL_INT(
      NEB_OK, neb_build_config_algreset(buffer, sizeof(buffer),
                                        NEB_ALGRESET_HEADING));
  TEST_ASSERT_EQUAL_STRING("CONFIG ALGRESET HEADING", buffer);
}

// Default threshold is 3 -- a no-op-ish value.
static neb_status_t act_heading_reliability(void) {
  return neb_heading_reliability(&g_handle, 3);
}

static neb_status_t act_heading_length_default(void) {
  return neb_heading_length_default(&g_handle);
}

// 100 cm baseline, 5 cm tolerance (RAM-only; power-cycle restores).
static neb_status_t act_heading_length(void) {
  return neb_heading_length(&g_handle, 100, 5);
}

static neb_status_t act_heading_mode(void) {
  return neb_heading_mode(&g_handle, NEB_HEADING2_FIXLENGTH);
}

// Zero correction -- valid and non-intrusive.
static neb_status_t act_heading_offset(void) {
  return neb_heading_offset(&g_handle, 0.0, 0.0);
}

// A mild 10 dB-Hz mask; the ack is what we verify, not the filtering effect.
static neb_status_t act_mask_cn0(void) { return neb_mask_cn0(&g_handle, 10); }

// Default 1200 s -- restores the documented default value.
static neb_status_t act_sbas_timeout(void) {
  return neb_rtk_sbas_timeout(&g_handle, 1200);
}

static neb_status_t act_mode_heading2(void) {
  return neb_mode_set_heading2(&g_handle, NEB_HEADING2_FIXLENGTH);
}

// ---------------------------------------------------------------------------
// The suite
// ---------------------------------------------------------------------------

typedef struct {
  const char *name;    // stable id, used in the results file
  const char *summary; // short label for the generated matrix
  neb_caps_t cap;      // 0 = supported by every model
  unsigned min_build;  // 0 = any build
  hil_level_t level;
  neb_status_t (*action)(void);
  void (*custom)(void);
  neb_status_t expected; // what a supporting device should return
  const char *note;
} hil_test_t;

static const hil_test_t g_tests[] = {
    // Read-only baselines; supported everywhere.
    {"mode_query", "MODE query (read)", 0, 0, HIL_LEVEL_READ, NULL,
     custom_mode_query, NEB_OK, NULL},
    {"config_query", "CONFIG query (read)", 0, 0, HIL_LEVEL_READ, NULL,
     custom_config_query, NEB_OK, NULL},
    {"mask_query", "MASK query (read)", NEB_CAP_MASK, 0, HIL_LEVEL_READ, NULL,
     custom_mask_query, NEB_OK, NULL},
    {"algreset_build_only", "CONFIG ALGRESET wire string (build-only)", 0, 0,
     HIL_LEVEL_READ, NULL, custom_algreset_build_only, NEB_OK, NULL},

    // §4.8 dual-antenna heading configuration.
    {"heading_reliability", "CONFIG HEADING RELIABILITY", NEB_CAP_HEADING, 0,
     HIL_LEVEL_RAM, act_heading_reliability, NULL, NEB_OK, NULL},
    // FIRMWARE DISCREPANCY (Manual §4.8 Table 4-13): the manual says omitting
    // both parameters uses the default configuration, but R4.10Build11826
    // rejects bare "CONFIG HEADING LENGTH" with "PARSING FAILD GRAMMAR ERROR".
    // Pinned as NEB_ERR_NAK: a future firmware that accepts it will fail this
    // test and prompt a revisit. The parameterized form IS accepted.
    {"heading_length_default", "CONFIG HEADING LENGTH (no params)",
     NEB_CAP_HEADING, 0, HIL_LEVEL_RAM, act_heading_length_default, NULL,
     NEB_ERR_NAK, "manual documents this as valid; firmware rejects it"},
    {"heading_length", "CONFIG HEADING LENGTH (with params)", NEB_CAP_HEADING,
     0, HIL_LEVEL_RAM, act_heading_length, NULL, NEB_OK, NULL},
    {"heading_mode", "CONFIG HEADING MODE", NEB_CAP_HEADING, 0, HIL_LEVEL_RAM,
     act_heading_mode, NULL, NEB_OK, NULL},

    // §4.9 heading/pitch offset (UM980/UM982).
    {"heading_offset", "CONFIG HEADING OFFSET", NEB_CAP_HEADING_OFFSET, 0,
     HIL_LEVEL_RAM, act_heading_offset, NULL, NEB_OK, NULL},

    // Build-gated on UM982 Build9669+.
    {"mask_cn0", "MASK CN0", NEB_CAP_MASK_CN0, 9669, HIL_LEVEL_RAM,
     act_mask_cn0, NULL, NEB_OK, NULL},
    {"sbas_timeout", "CONFIG SBAS TIMEOUT", NEB_CAP_SBAS_TIMEOUT, 9669,
     HIL_LEVEL_RAM, act_sbas_timeout, NULL, NEB_OK, NULL},

    // Last: this one changes the operating mode.
    {"mode_heading2", "MODE HEADING2", NEB_CAP_HEADING2_MODE, 0, HIL_LEVEL_RAM,
     act_mode_heading2, NULL, NEB_OK, NULL},
};

// What the harness decided to do with the current test.
typedef enum {
  PLAN_EXPECT,      // model supports it: assert `expected`
  PLAN_UNSUPPORTED, // model lacks it: assert the gate refuses it
  PLAN_SKIP_BUILD,
  PLAN_SKIP_LEVEL
} hil_plan_t;

static const hil_test_t *g_current;
static hil_plan_t g_plan;
static char g_skip_reason[128];

static void dispatch(void) {
  switch (g_plan) {
  case PLAN_SKIP_LEVEL:
    TEST_IGNORE_MESSAGE("changes configuration; NEB_TEST_LEVEL=read");
    break;
  case PLAN_SKIP_BUILD:
    TEST_IGNORE_MESSAGE(g_skip_reason);
    break;
  case PLAN_UNSUPPORTED:
    // No wire traffic: the capability gate returns before framing anything.
    if (g_current->action)
      TEST_ASSERT_EQUAL_INT(NEB_ERR_UNSUPPORTED, g_current->action());
    else
      TEST_IGNORE_MESSAGE("not applicable to this model");
    break;
  case PLAN_EXPECT:
  default:
    if (g_current->custom)
      g_current->custom();
    else
      TEST_ASSERT_EQUAL_INT(g_current->expected, g_current->action());
    break;
  }
}

int main(void) {
  const char *port = getenv("NEB_TEST_PORT");
  if (!port) {
    fprintf(stderr, "NEB_TEST_PORT is not set. The port is never guessed.\n");
    return 2;
  }

  const char *baud_text = getenv("NEB_TEST_BAUD");
  const int baudrate = baud_text ? (int)strtol(baud_text, NULL, 10) : 115200;

  const char *board = getenv("NEB_TEST_BOARD");
  if (!board || !board[0])
    fprintf(stderr,
            "WARNING: NEB_TEST_BOARD is not set. The receiver can report its "
            "model but not what board it is mounted on, so the recorded row "
            "will say \"(unspecified)\". Set it, e.g.\n"
            "  NEB_TEST_BOARD=\"Holybro H-RTK Unicore UM982\"\n\n");

  const char *level_text = getenv("NEB_TEST_LEVEL");
  const hil_level_t level =
      (level_text && strcmp(level_text, "read") == 0) ? HIL_LEVEL_READ
                                                      : HIL_LEVEL_RAM;

  // Identify before deciding anything: the model drives capability gating, and
  // opening with the wrong one mis-sets the bitfield silently rather than
  // erroring.
  hil_device_t device;
  if (hil_identify(port, baudrate, &device) != 0) {
    fprintf(stderr,
            "Could not identify a receiver on %s at %d baud.\n"
            "VERSIONA did not return a recognizable model. Check the port, the "
            "baud rate, and that nothing else holds the device.\n",
            port, baudrate);
    return 1;
  }

  fprintf(stderr, "Device: %s %s (build %u) on %s @ %d, level=%s\n",
          device.model_name, device.firmware, device.build, port, baudrate,
          level == HIL_LEVEL_RAM ? "ram" : "read");

  neb_status_t status = neb_open(&g_handle, device.model, port, baudrate);
  if (status != NEB_OK) {
    fprintf(stderr, "neb_open(%s): %s\n", port, neb_strerror(status));
    return 1;
  }

  const neb_caps_t caps = neb_caps_for_model(device.model);
  int sent_ram_commands = 0;

  UNITY_BEGIN();
  for (size_t i = 0; i < sizeof(g_tests) / sizeof(g_tests[0]); i++) {
    const hil_test_t *test = &g_tests[i];
    g_current = test;

    const int supported = (test->cap == 0) || ((caps & test->cap) != 0);

    if (!supported)
      g_plan = PLAN_UNSUPPORTED;
    else if (test->level > level)
      g_plan = PLAN_SKIP_LEVEL;
    else if (test->min_build && device.build < test->min_build) {
      g_plan = PLAN_SKIP_BUILD;
      snprintf(g_skip_reason, sizeof(g_skip_reason),
               "needs Build%u+, device reports Build%u", test->min_build,
               device.build);
    } else {
      g_plan = PLAN_EXPECT;
      if (test->level == HIL_LEVEL_RAM)
        sent_ram_commands = 1;
    }

    const unsigned failures_before = Unity.TestFailures;
    const unsigned ignores_before = Unity.TestIgnores;

    UnityDefaultTestRun(dispatch, test->name, __LINE__);

    hil_outcome_t outcome;
    const char *detail = test->note;

    if (Unity.TestFailures > failures_before)
      outcome = HIL_FAIL;
    else if (Unity.TestIgnores > ignores_before)
      outcome = (g_plan == PLAN_SKIP_BUILD) ? HIL_SKIP_BUILD : HIL_SKIP_LEVEL;
    else if (g_plan == PLAN_UNSUPPORTED) {
      outcome = HIL_UNSUPPORTED;
      detail = "capability gate refused it, as expected";
    } else if (test->expected != NEB_OK)
      outcome = HIL_DISCREPANCY;
    else
      outcome = HIL_PASS;

    hil_record(test->name, test->summary, outcome, detail);
  }
  const int result = UNITY_END();

  neb_close(&g_handle);

  const char *results_dir = getenv("NEB_TEST_RESULTS_DIR");
  hil_write_results(&device, results_dir ? results_dir : "tests/results",
                    board, baudrate, level, getenv("NEB_TEST_CONTRIBUTOR"));

  if (sent_ram_commands)
    fprintf(stderr,
            "\nNOTE: RAM-only changes were sent (no SAVECONFIG). Power-cycle "
            "the receiver to restore its saved configuration.\n");

  return result;
}
