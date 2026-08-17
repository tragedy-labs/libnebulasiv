// neb_mode.h
//
// MODE-family commands (Manual §3): the receiver's operating mode -- base,
// rover, heading. The manual treats MODE as its own command type distinct from
// CONFIG, so it lives in its own capability module.
//
// Each command is split into a pure `neb_build_*` function (formats the exact
// wire string, validates parameters, no I/O) and a thin `neb_mode_*` wrapper
// (capability check, build, send).
#ifndef NEB_MODE_H
#define NEB_MODE_H

#include <stddef.h>
#include <stdint.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_rover_profile_t, neb_heading2_mode_t

// --- Builders (pure; no handle, no transport) -----------------------------
// Each writes a NUL-terminated command into `out` and returns NEB_OK, or
// NEB_ERR_INVALID_PARAM for a bad parameter / NEB_ERR_OVERFLOW if it would not
// fit in `len`.

neb_status_t neb_build_mode_query(char *out, size_t len);
neb_status_t neb_build_mode_set_rover(char *out, size_t len);
neb_status_t neb_build_mode_set_rover_profile(char *out, size_t len,
                                              neb_rover_profile_t profile);
neb_status_t neb_build_mode_set_base_auto(char *out, size_t len);
neb_status_t neb_build_mode_set_base_id(char *out, size_t len, uint16_t id);
neb_status_t neb_build_mode_set_base_self_optimize(char *out, size_t len,
                                                   unsigned time_s);
neb_status_t neb_build_mode_set_base_self_optimize_dist(char *out, size_t len,
                                                        unsigned time_s,
                                                        double distance_m);
neb_status_t neb_build_mode_set_base_fixed(char *out, size_t len,
                                           double lat_deg, double lon_deg,
                                           double alt_m);
neb_status_t neb_build_mode_set_heading2(char *out, size_t len,
                                         neb_heading2_mode_t mode);

// --- Wrappers (capability check + build + send) ---------------------------

// Query the current operating mode (Manual §3.1). On success `response`
// receives the raw "#MODE,...;MODE ROVER SURVEY,*hh" record; pass NULL if the
// text is not needed.
neb_status_t neb_mode_query(neb_handle_t *handle, char *response,
                            size_t response_size);

// Put the receiver into rover mode with its model-default profile (Manual
// §3.6). The receiver auto-detects RTCM format, so no correction type is
// specified.
neb_status_t neb_mode_set_rover(neb_handle_t *handle);

// Put the receiver into rover mode with an explicit dynamic profile such as
// UAV or surveying (Manual §3.6). Use when you know the vehicle type at the
// call site; for the model default, use neb_mode_set_rover().
//
// Build-gated: the profile parameters require a minimum firmware build (Manual
// §3.6: UM980 Build7923+, UM982 Build7650+). The capability check is
// model-level only; if the running build lacks support the device replies
// NEB_ERR_NAK.
neb_status_t neb_mode_set_rover_profile(neb_handle_t *handle,
                                        neb_rover_profile_t profile);

// Put the receiver into base-station mode using its default self-survey
// (Manual §3.4): it averages 60 s of fixes (or until the error tolerance is
// met) and uses that as the base coordinate.
neb_status_t neb_mode_set_base_auto(neb_handle_t *handle);

// Put the receiver into base-station mode and assign it a station ID
// (Manual §3.5). Valid IDs are 0..4095; larger returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_mode_set_base_id(neb_handle_t *handle, uint16_t id);

// Put the receiver into self-optimizing base-station mode (Manual §3.3),
// averaging the position for up to `time_s` seconds. To also pin the result to
// a previously saved coordinate within a tolerance, use the _dist variant.
neb_status_t neb_mode_set_base_self_optimize(neb_handle_t *handle,
                                             unsigned time_s);

// Self-optimizing base mode with a distance tolerance (Manual §3.3): if the new
// optimized coordinate is within `distance_m` of the one saved in flash, the
// saved coordinate is reused. Valid distance is 0..10 m (0 means "always use
// the newly optimized result"); out of range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_mode_set_base_self_optimize_dist(neb_handle_t *handle,
                                                  unsigned time_s,
                                                  double distance_m);

// Put the receiver into fixed base-station mode with precise geodetic
// coordinates (Manual §3.2). Bounds are validated before anything is sent:
// -90..90 lat, -180..180 lon, -30000..30000 m altitude; out of range returns
// NEB_ERR_INVALID_PARAM. (The ECEF coordinate form is a separate builder,
// added later.)
neb_status_t neb_mode_set_base_fixed(neb_handle_t *handle, double lat_deg,
                                     double lon_deg, double alt_m);

// Put the receiver into dual-receiver heading mode with the given baseline
// model (Manual §3.7). Not available on the UM960L; returns
// NEB_ERR_UNSUPPORTED there. Passing NEB_HEADING2_FIXLENGTH matches the
// receiver's own default for a bare "MODE HEADING2".
neb_status_t neb_mode_set_heading2(neb_handle_t *handle,
                                   neb_heading2_mode_t mode);

#endif
