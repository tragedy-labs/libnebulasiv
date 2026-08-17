// main.c -- minimal example driving a Unicore UM980 over the serial layer.
//
// Demonstrates: opening a handle, a MODE query, PPP enable/disable, a PPP
// CONVERGE command, and error handling via neb_status_t / neb_strerror().
//
// NOTE: PPP enable/disable/converge change the receiver's live configuration
// (they are not persisted -- no SAVECONFIG is ever sent -- and revert on
// power cycle).
#include "um980.h"

#include <stdio.h>

static void report(const char *what, neb_status_t st) {
  printf("%-28s -> %s\n", what, neb_strerror(st));
}

int main(void) {
  neb_handle_t gps;

  neb_status_t st = neb_open(&gps, NEB_MODEL_UM980, "/dev/ttyUSB0", 115200);
  if (st != NEB_OK) {
    fprintf(stderr, "neb_open: %s\n", neb_strerror(st));
    return 1;
  }

  // --- Query the operating mode; capture and print the returned record. ---
  char mode[256];
  st = neb_mode_query(&gps, mode, sizeof(mode));
  report("MODE query", st);
  if (st == NEB_OK)
    printf("    reply: %s\n", mode);

  // --- Parameter validation: caught before anything hits the wire. ---
  st = neb_config_ppp_converge(&gps, -1.0, 20.0);
  report("PPP CONVERGE (bad param)", st); // expect NEB_ERR_INVALID_PARAM

  // --- Enable PPP, tune convergence, then disable. ---
  st = neb_config_ppp_enable(&gps, NEB_PPP_B2B);
  report("PPP enable (B2b-PPP)", st);

  st = neb_config_ppp_converge(&gps, 10.0, 20.0);
  report("PPP CONVERGE 10 20", st);

  st = neb_config_ppp_disable(&gps);
  report("PPP disable", st);

  neb_close(&gps);
  return 0;
}
