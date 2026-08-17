#include "neb_mode.h"
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O. Each formats the exact wire string documented in
// the manual and validates its parameters against the documented bounds.
// ===========================================================================

// Manual §3.1 -- Query the Receiver's Operating Mode
neb_status_t neb_build_mode_query(char *out, size_t len) {
  // "MODE"
  int n = snprintf(out, len, "%s", NEB_CMD_MODE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.6 -- Rover Station Mode (model default)
neb_status_t neb_build_mode_set_rover(char *out, size_t len) {
  // "MODE ROVER"
  int n = snprintf(out, len, "%s %s", NEB_CMD_MODE, NEB_TOK_ROVER);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.6 -- Rover Station Mode with an explicit dynamic profile
neb_status_t neb_build_mode_set_rover_profile(char *out, size_t len,
                                              neb_rover_profile_t profile) {
  const char *profile_str = neb_rover_profile_str(profile);
  if (!profile_str)
    return NEB_ERR_INVALID_PARAM;

  // "MODE ROVER <profile>"
  int n =
      snprintf(out, len, "%s %s %s", NEB_CMD_MODE, NEB_TOK_ROVER, profile_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.4 -- Base Station Mode without Parameters (default self-survey)
neb_status_t neb_build_mode_set_base_auto(char *out, size_t len) {
  // "MODE BASE"
  int n = snprintf(out, len, "%s %s", NEB_CMD_MODE, NEB_TOK_BASE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.5 -- Base Station ID
neb_status_t neb_build_mode_set_base_id(char *out, size_t len, uint16_t id) {
  // Documented range: 0 <= ID < 4096 (Manual §3.5, Table 3-7).
  if (id > 4095)
    return NEB_ERR_INVALID_PARAM;

  // "MODE BASE <id>"
  int n =
      snprintf(out, len, "%s %s %u", NEB_CMD_MODE, NEB_TOK_BASE, (unsigned)id);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.3 -- Self-optimizing Base Station Mode
neb_status_t neb_build_mode_set_base_self_optimize(char *out, size_t len,
                                                   unsigned time_s) {
  // Time must be non-negative (guaranteed by the unsigned type); the manual
  // gives no documented upper bound, so none is imposed here.
  // "MODE BASE TIME <t>"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_MODE, NEB_TOK_BASE,
                   NEB_TOK_TIME, time_s);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.3 -- Self-optimizing Base Station Mode with distance tolerance
neb_status_t neb_build_mode_set_base_self_optimize_dist(char *out, size_t len,
                                                        unsigned time_s,
                                                        double distance_m) {
  // Documented range: 0 <= Distance <= 10 metres (Manual §3.3, Table 3-5).
  if (distance_m < 0.0 || distance_m > 10.0)
    return NEB_ERR_INVALID_PARAM;

  // "MODE BASE TIME <t> <d>"
  int n = snprintf(out, len, "%s %s %s %u %g", NEB_CMD_MODE, NEB_TOK_BASE,
                   NEB_TOK_TIME, time_s, distance_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.2 -- Fixed Base Station with Precise Coordinates
neb_status_t neb_build_mode_set_base_fixed(char *out, size_t len,
                                           double lat_deg, double lon_deg,
                                           double alt_m) {
  // Documented geodetic bounds (Manual §3.2, Table 3-4).
  if (lat_deg < -90.0 || lat_deg > 90.0)
    return NEB_ERR_INVALID_PARAM;
  if (lon_deg < -180.0 || lon_deg > 180.0)
    return NEB_ERR_INVALID_PARAM;
  if (alt_m < -30000.0 || alt_m > 30000.0)
    return NEB_ERR_INVALID_PARAM;

  // "MODE BASE <lat> <lon> <alt>". Fixed-point (never %g) so a coordinate near
  // the equator/meridian can't render in scientific notation on the wire. The
  // manual's examples carry 11 decimal places for the geodetic coordinates and
  // 4 for altitude (Manual §3.2), so %.11f / %.4f reproduce that precision
  // losslessly for a real survey coordinate.
  int n = snprintf(out, len, "%s %s %.11f %.11f %.4f", NEB_CMD_MODE,
                   NEB_TOK_BASE, lat_deg, lon_deg, alt_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §3.7 -- Heading2 Mode
neb_status_t neb_build_mode_set_heading2(char *out, size_t len,
                                         neb_heading2_mode_t mode) {
  const char *mode_str = neb_heading2_mode_str(mode);
  if (!mode_str)
    return NEB_ERR_INVALID_PARAM;

  // "MODE HEADING2 <mode>"
  int n =
      snprintf(out, len, "%s %s %s", NEB_CMD_MODE, NEB_TOK_HEADING2, mode_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send.
// ===========================================================================

neb_status_t neb_mode_query(neb_handle_t *handle, char *response,
                            size_t response_size) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_query(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, response, response_size);
}

neb_status_t neb_mode_set_rover(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_rover(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_rover_profile(neb_handle_t *handle,
                                        neb_rover_profile_t profile) {
  if (!neb_has_cap(handle, NEB_CAP_ROVER_PROFILE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_rover_profile(cmd, sizeof(cmd), profile);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_base_auto(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_base_auto(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_base_id(neb_handle_t *handle, uint16_t id) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_base_id(cmd, sizeof(cmd), id);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_base_self_optimize(neb_handle_t *handle,
                                             unsigned time_s) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_mode_set_base_self_optimize(cmd, sizeof(cmd), time_s);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_base_self_optimize_dist(neb_handle_t *handle,
                                                  unsigned time_s,
                                                  double distance_m) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_base_self_optimize_dist(
      cmd, sizeof(cmd), time_s, distance_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_base_fixed(neb_handle_t *handle, double lat_deg,
                                     double lon_deg, double alt_m) {
  if (!neb_has_cap(handle, NEB_CAP_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_mode_set_base_fixed(cmd, sizeof(cmd), lat_deg, lon_deg, alt_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mode_set_heading2(neb_handle_t *handle,
                                   neb_heading2_mode_t mode) {
  if (!neb_has_cap(handle, NEB_CAP_HEADING2_MODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mode_set_heading2(cmd, sizeof(cmd), mode);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
