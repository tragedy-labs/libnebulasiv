// neb_assist.h
//
// Assisted position and time input (Manual §6): AGNSS aiding that shortens the
// time-to-first-fix. Unlike MODE/CONFIG, these are NMEA-style, comma-separated,
// "$"-prefixed commands.
//
// Applicable to UM980 and UM982 only (they gate on NEB_CAP_AGNSS); other models
// return NEB_ERR_UNSUPPORTED.
#ifndef NEB_ASSIST_H
#define NEB_ASSIST_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_lat_dir_t, neb_lon_dir_t

// --- Builders (pure; no handle, no transport) -----------------------------

// Assisted position (Manual §6.1). Latitude/longitude are in NMEA
// ddmm.mmmmmm / dddmm.mmmmmm format (degrees*100 + decimal minutes), with the
// hemisphere given separately -- NOT decimal degrees. Altitude is ellipsoidal
// height in metres. The assisted position must be within 10000 m of the true
// position for the device to use it. Validation (else NEB_ERR_INVALID_PARAM):
// latitude 0..9000 and longitude 0..18000 with the minutes part < 60.
neb_status_t neb_build_assist_position(char *out, size_t len, double lat_ddmm,
                                       neb_lat_dir_t lat_dir, double lon_ddmm,
                                       neb_lon_dir_t lon_dir, double alt_m);

// Assisted time (Manual §6.2), UTC within +/- 3 s. Validation (else
// NEB_ERR_INVALID_PARAM): month 1..12, day 1..31, hour 0..23, minute 0..59,
// second 0..60 (0..60 allows a leap second), millisecond 0..999. Year and
// leap-second count are passed through.
neb_status_t neb_build_assist_time(char *out, size_t len, unsigned year,
                                   unsigned month, unsigned day, unsigned hour,
                                   unsigned minute, unsigned second,
                                   unsigned millisecond, unsigned leapsec);

// --- Wrappers (capability check + build + send) ---------------------------

neb_status_t neb_assist_position(neb_handle_t *handle, double lat_ddmm,
                                 neb_lat_dir_t lat_dir, double lon_ddmm,
                                 neb_lon_dir_t lon_dir, double alt_m);
neb_status_t neb_assist_time(neb_handle_t *handle, unsigned year,
                             unsigned month, unsigned day, unsigned hour,
                             unsigned minute, unsigned second,
                             unsigned millisecond, unsigned leapsec);

#endif
