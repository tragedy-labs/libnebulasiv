// test_hil_um980.c
//
// Hardware-in-the-loop smoke test against a real UM980 at $NEB_TEST_PORT. Run
// only via `make test-hardware` -- never part of `make test`.
//
// Deliberately minimal and conservative: only read-only, non-mutating query
// commands. Nothing here changes persistent config; no SAVECONFIG, no FRESET.
// Expand this list only with commands confirmed non-mutating per the manual.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neb_config.h"
#include "neb_core.h"
#include "neb_mode.h"

static neb_handle_t g_handle;

void setUp(void) {}
void tearDown(void) {}

// MODE query -- read-only (Manual §3.1).
static void test_hil_mode_query(void) {
  char resp[256];
  TEST_ASSERT_EQUAL_INT(NEB_OK, neb_mode_query(&g_handle, resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "response: OK"));
  TEST_ASSERT_NOT_NULL(strstr(resp, "#MODE,"));
}

// CONFIG query -- read-only (Manual §4.1).
static void test_hil_config_query(void) {
  char resp[1024];
  TEST_ASSERT_EQUAL_INT(NEB_OK,
                        neb_config_query(&g_handle, resp, sizeof(resp)));
  TEST_ASSERT_NOT_NULL(strstr(resp, "response: OK"));
}

int main(void) {
  const char *port = getenv("NEB_TEST_PORT");
  if (!port) {
    fprintf(stderr, "NEB_TEST_PORT not set\n");
    return 2;
  }

  neb_status_t st = neb_open(&g_handle, NEB_MODEL_UM980, port, 115200);
  if (st != NEB_OK) {
    fprintf(stderr, "neb_open(%s): %s\n", port, neb_strerror(st));
    return 1;
  }

  UNITY_BEGIN();
  RUN_TEST(test_hil_mode_query);
  RUN_TEST(test_hil_config_query);
  int rc = UNITY_END();

  neb_close(&g_handle);
  return rc;
}
