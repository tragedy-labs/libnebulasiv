// hil_device.h
//
// Shared harness for the hardware-in-the-loop suite.
//
// Nothing here assumes a particular receiver, port, baud rate, or host. The
// port is supplied by the caller (NEB_TEST_PORT); the model and firmware build
// are read off whatever answered, with VERSIONA. That means one suite runs
// against any N4 board -- what changes per device is what each test is
// expected to do, not which file gets compiled.
//
// Each run is recorded as a machine-readable result file under
// tests/results/. Those files are the record of what was verified on what;
// the matrix in HARDWARE_TESTING.md is generated from them and should never be
// edited by hand.
#ifndef HIL_DEVICE_H
#define HIL_DEVICE_H

#include "neb_core.h"

// What VERSIONA reported about the attached receiver.
typedef struct {
  neb_model_t model;
  char model_name[16]; // "UM980", "UM982", ...
  char firmware[64];   // "R4.10Build13504" as reported
  unsigned build;      // 13504 -- the number build-gated commands compare to
} hil_device_t;

// How invasive a test is. A run declares the highest level it permits; tests
// above that are skipped and recorded as such.
typedef enum {
  HIL_LEVEL_READ = 0, // read-only queries and pure builders; changes nothing
  HIL_LEVEL_RAM = 1   // changes live configuration (RAM only, never SAVECONFIG)
} hil_level_t;

// What happened to one test. Only SKIP_BUILD and SKIP_LEVEL mean "we learned
// nothing" -- UNSUPPORTED is a positive result, confirming the library refused
// a command this model does not have.
typedef enum {
  HIL_PASS,
  HIL_FAIL,
  HIL_UNSUPPORTED,  // capability gate refused it, as expected for this model
  HIL_DISCREPANCY,  // pinned deviation from the manual, reproduced on device
  HIL_SKIP_BUILD,   // model has the command, firmware build is below minimum
  HIL_SKIP_LEVEL    // more invasive than this run permits
} hil_outcome_t;

// Identify the receiver reachable on `port` at `baudrate`. Opens, sends
// VERSIONA, scrapes the model and build, closes. Returns 0 on success, -1 if
// the port could not be opened or the reply could not be understood.
int hil_identify(const char *port, int baudrate, hil_device_t *device);

// Record one test's outcome. `summary` is the human label that appears in the
// generated matrix; `detail` may be NULL.
void hil_record(const char *test, const char *summary, hil_outcome_t outcome,
                const char *detail);

// Write the run to <dir>/<model>-build<n>-<board>.tsv. Returns 0 on success.
// The file is written to a temporary and renamed, so an interrupted run leaves
// the previous record intact rather than a half-written one.
int hil_write_results(const hil_device_t *device, const char *dir,
                      const char *board, int baudrate, hil_level_t level,
                      const char *contributor);

#endif
