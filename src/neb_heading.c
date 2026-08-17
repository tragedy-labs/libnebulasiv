#include "neb_heading.h"
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O.
//
// NOTE: the §4.8 builders (mode / reliability / length) are ALPHA -- UM982-only
// and unverified on hardware (no UM982 available). Their wire strings are
// unit-tested against the manual, but no real device has parsed them.
// neb_build_heading_offset (§4.9) is UM980/UM982 and hardware-checkable.
// ===========================================================================

// ALPHA. Manual §4.8, Table 4-12 -- CONFIG HEADING <mode>
neb_status_t neb_build_heading_mode(char *out, size_t len,
                                    neb_heading2_mode_t mode) {
  const char *mode_str = neb_heading2_mode_str(mode);
  if (!mode_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG HEADING FIXLENGTH"
  int n =
      snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_HEADING, mode_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ALPHA. Manual §4.8 -- CONFIG HEADING RELIABILITY <threshold>
neb_status_t neb_build_heading_reliability(char *out, size_t len,
                                           unsigned threshold) {
  // Documented range 1..4 (default 3).
  if (threshold < 1 || threshold > 4)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG HEADING RELIABILITY 3"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_HEADING,
                   NEB_TOK_RELIABILITY, threshold);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ALPHA. Manual §4.8, Table 4-13 -- CONFIG HEADING LENGTH (defaults)
neb_status_t neb_build_heading_length_default(char *out, size_t len) {
  // "CONFIG HEADING LENGTH"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_HEADING,
                   NEB_TOK_LENGTH);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ALPHA. Manual §4.8, Table 4-13 -- CONFIG HEADING LENGTH <length> <tolerance>
neb_status_t neb_build_heading_length(char *out, size_t len, unsigned length_cm,
                                      unsigned tolerance_cm) {
  // A baseline length must be positive; the manual states no numeric bounds
  // beyond that, so the device validates the upper end.
  if (length_cm < 1)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG HEADING LENGTH 20 3"
  int n = snprintf(out, len, "%s %s %s %u %u", NEB_CMD_CONFIG, NEB_TOK_HEADING,
                   NEB_TOK_LENGTH, length_cm, tolerance_cm);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.9, Table 4-14 -- CONFIG HEADING OFFSET <heading> <pitch>
neb_status_t neb_build_heading_offset(char *out, size_t len, double heading_deg,
                                      double pitch_deg) {
  // Documented ranges: heading -180..180, pitch -90..90 degrees.
  if (heading_deg < -180.0 || heading_deg > 180.0)
    return NEB_ERR_INVALID_PARAM;
  if (pitch_deg < -90.0 || pitch_deg > 90.0)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG HEADING OFFSET 90 45"
  int n = snprintf(out, len, "%s %s %s %g %g", NEB_CMD_CONFIG, NEB_TOK_HEADING,
                   NEB_TOK_OFFSET, heading_deg, pitch_deg);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send.
// ===========================================================================

neb_status_t neb_heading_mode(neb_handle_t *handle, neb_heading2_mode_t mode) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_heading_mode(cmd, sizeof(cmd), mode);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_heading_reliability(neb_handle_t *handle, unsigned threshold) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_heading_reliability(cmd, sizeof(cmd), threshold);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_heading_length_default(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_heading_length_default(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_heading_length(neb_handle_t *handle, unsigned length_cm,
                                unsigned tolerance_cm) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_heading_length(cmd, sizeof(cmd), length_cm, tolerance_cm);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_heading_offset(neb_handle_t *handle, double heading_deg,
                                double pitch_deg) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING_OFFSET))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_heading_offset(cmd, sizeof(cmd), heading_deg, pitch_deg);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
