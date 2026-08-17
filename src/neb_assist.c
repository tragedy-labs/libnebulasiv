#include "neb_assist.h"
#include "neb_protocol.h"

#include <stdio.h>

// A valid NMEA ddmm.mmmmmm magnitude: non-negative, whole degrees <= max_deg,
// and the minutes part below 60. Computed without libm (no fmod).
static int valid_ddmm(double ddmm, double max_deg) {
  if (!(ddmm >= 0.0))
    return 0;
  double degrees = (double)(long)(ddmm / 100.0);
  double minutes = ddmm - degrees * 100.0;
  if (minutes < 0.0 || minutes >= 60.0)
    return 0;
  if (degrees > max_deg)
    return 0;
  return 1;
}

// Manual §6.1 -- $AIDPOS,Latitude,LatDir,Longitude,LonDir,Altitude
neb_status_t neb_build_assist_position(char *out, size_t len, double lat_ddmm,
                                       neb_lat_dir_t lat_dir, double lon_ddmm,
                                       neb_lon_dir_t lon_dir, double alt_m) {
  const char *lat_dir_str = neb_lat_dir_str(lat_dir);
  const char *lon_dir_str = neb_lon_dir_str(lon_dir);
  if (!lat_dir_str || !lon_dir_str)
    return NEB_ERR_INVALID_PARAM;
  if (!valid_ddmm(lat_ddmm, 90.0) || !valid_ddmm(lon_ddmm, 180.0))
    return NEB_ERR_INVALID_PARAM;
  // Altitude in metres. The manual states no range; bound the magnitude to a
  // generous finite sanity range (rejects NaN/Inf and absurd values, and keeps
  // the fixed-point %.3f output below from overflowing the command buffer).
  if (!(alt_m > -1e6 && alt_m < 1e6))
    return NEB_ERR_INVALID_PARAM;

  // "$AIDPOS,4002.229934,N,11618.096855,E,37.254" -- %.3f matches the manual's
  // altitude format and, unlike %g, never emits scientific notation.
  int n = snprintf(out, len, "%s,%.6f,%s,%.6f,%s,%.3f", NEB_CMD_AIDPOS,
                   lat_ddmm, lat_dir_str, lon_ddmm, lon_dir_str, alt_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §6.2 -- $AIDTIME,Year,Month,Day,Hour,Minute,Second,Millisecond,Leapsec
neb_status_t neb_build_assist_time(char *out, size_t len, unsigned year,
                                   unsigned month, unsigned day, unsigned hour,
                                   unsigned minute, unsigned second,
                                   unsigned millisecond, unsigned leapsec) {
  // Calendar-range validation (the manual types these as UINT without stating
  // ranges; these are basic calendar bounds, not invented device rules).
  if (month < 1 || month > 12)
    return NEB_ERR_INVALID_PARAM;
  if (day < 1 || day > 31)
    return NEB_ERR_INVALID_PARAM;
  if (hour > 23)
    return NEB_ERR_INVALID_PARAM;
  if (minute > 59)
    return NEB_ERR_INVALID_PARAM;
  if (second > 60) // 60 permits a leap second
    return NEB_ERR_INVALID_PARAM;
  if (millisecond > 999)
    return NEB_ERR_INVALID_PARAM;

  // "$AIDTIME,2021,12,3,15,2,36,400,18"
  int n =
      snprintf(out, len, "%s,%u,%u,%u,%u,%u,%u,%u,%u", NEB_CMD_AIDTIME, year,
               month, day, hour, minute, second, millisecond, leapsec);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send. Assisted commands are UM980/UM982.
// ===========================================================================

neb_status_t neb_assist_position(neb_handle_t *handle, double lat_ddmm,
                                 neb_lat_dir_t lat_dir, double lon_ddmm,
                                 neb_lon_dir_t lon_dir, double alt_m) {
  if (!neb_has_cap(handle, NEB_CAP_AGNSS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_assist_position(
      cmd, sizeof(cmd), lat_ddmm, lat_dir, lon_ddmm, lon_dir, alt_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_assist_time(neb_handle_t *handle, unsigned year,
                             unsigned month, unsigned day, unsigned hour,
                             unsigned minute, unsigned second,
                             unsigned millisecond, unsigned leapsec) {
  if (!neb_has_cap(handle, NEB_CAP_AGNSS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_assist_time(cmd, sizeof(cmd), year, month, day, hour, minute,
                            second, millisecond, leapsec);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
