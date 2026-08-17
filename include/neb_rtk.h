// neb_rtk.h
//
// RTK / differential correction-stream commands (Manual §4.5-§4.7, §4.10,
// §4.26): DGPS and RTK data timeouts, RTK reliability and solution control, and
// (later) STANDALONE, SBAS, and base-antenna configuration. These are the
// commands that shape how the receiver consumes and trusts correction data.
//
// Same split as the other modules: a pure `neb_build_*` builder plus a thin
// `neb_rtk_*` wrapper. DGPS and RTK are available on every model; STANDALONE
// and SBAS are not on the UM960L, and SBAS TIMEOUT is UM982-only -- each
// wrapper gates on the appropriate capability flag.
#ifndef NEB_RTK_H
#define NEB_RTK_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_sbas_system_t

// --- Builders (pure; no handle, no transport) -----------------------------

// DGPS differential-data timeout (Manual §4.5, Table 4-8). 0 disables DGPS;
// 1..1800 s sets the maximum age (default 300). Out of range returns
// NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_dgps_timeout(char *out, size_t len,
                                        unsigned seconds);

// RTK data timeout (Manual §4.6, Table 4-9). 0 disables RTK; 1..1800 s sets the
// maximum age (default UM982 600, UM980 120). Out of range returns
// NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_timeout(char *out, size_t len, unsigned seconds);

// RTK reliability thresholds (Manual §4.6, Table 4-10). `engine` is the RTK
// engine threshold 1..4 (default 3); `adr` is the ADR threshold 1..4 (default
// 1; values 2 and 3 are reserved). Out of range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_reliability(char *out, size_t len, unsigned engine,
                                       unsigned adr);

// Restore the default RTK dynamic mode (Manual §4.6). Wire token verified on a
// UM980 as "USER_DEFAULTS".
neb_status_t neb_build_rtk_user_defaults(char *out, size_t len);

// Reset the RTK solution (Manual §4.6). Disrupts the current RTK fix.
neb_status_t neb_build_rtk_reset(char *out, size_t len);

// Stop calculating RTK results, float and fixed (Manual §4.6).
neb_status_t neb_build_rtk_disable(char *out, size_t len);

// STANDALONE mode (Manual §4.7). Disable, or enable in one of three forms:
// default (no params), with a known coordinate, or with a wait time.
neb_status_t neb_build_rtk_standalone_disable(char *out, size_t len);
neb_status_t neb_build_rtk_standalone_enable(char *out, size_t len);
// Coordinates: -90..90 lat, -180..180 lon, -30000..18000 m alt (Table 4-11;
// note the altitude ceiling is 18000, unlike MODE BASE's 30000). Out of range
// returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_standalone_enable_coords(char *out, size_t len,
                                                    double lat_deg,
                                                    double lon_deg,
                                                    double alt_m);
// Wait time before auto-entering standalone: 3..100 s (default 100). Out of
// range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_standalone_enable_time(char *out, size_t len,
                                                  unsigned seconds);

// SBAS (Manual §4.10). Enable with a system selection, or disable.
neb_status_t neb_build_rtk_sbas_enable(char *out, size_t len,
                                       neb_sbas_system_t system);
neb_status_t neb_build_rtk_sbas_disable(char *out, size_t len);
// SBAS timeout, 120..1800 s (default 1200). Out of range returns
// NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_sbas_timeout(char *out, size_t len,
                                        unsigned seconds);

// Base-station antenna model (Manual §4.26). Sets the antenna `name` (quoted on
// the wire; may contain spaces per the RTCM/IGS convention), serial number
// `sn`, `setup_id`, and `type`, affecting RTCM 1005/1006/1007/1033 output.
// Validation (else NEB_ERR_INVALID_PARAM): `name` and `sn` are 1..31 printable
// ASCII characters; neither may contain a double-quote (which would break the
// wire quoting) and `sn` may not contain a space (it is not quoted); `setup_id`
// is 0..255.
neb_status_t neb_build_rtk_base_antenna(char *out, size_t len, const char *name,
                                        const char *sn, unsigned setup_id,
                                        neb_antenna_type_t type);

// Base-station antenna height and plane offset (Manual §4.22, Table 4-30),
// affecting the RTCM 1006 message. All in metres: height 0..6.5535, east and
// north 0..100 (all default 0). Out of range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_rtk_antenna_delta(char *out, size_t len, double height_m,
                                         double east_m, double north_m);

// --- Wrappers (capability check + build + send) ---------------------------

neb_status_t neb_rtk_dgps_timeout(neb_handle_t *handle, unsigned seconds);
neb_status_t neb_rtk_timeout(neb_handle_t *handle, unsigned seconds);
neb_status_t neb_rtk_reliability(neb_handle_t *handle, unsigned engine,
                                 unsigned adr);
neb_status_t neb_rtk_user_defaults(neb_handle_t *handle);
neb_status_t neb_rtk_reset(neb_handle_t *handle);
neb_status_t neb_rtk_disable(neb_handle_t *handle);

// STANDALONE (Manual §4.7). Applicable to UM960/UM980/UM982; the UM960L
// returns NEB_ERR_UNSUPPORTED.
neb_status_t neb_rtk_standalone_disable(neb_handle_t *handle);
neb_status_t neb_rtk_standalone_enable(neb_handle_t *handle);
neb_status_t neb_rtk_standalone_enable_coords(neb_handle_t *handle,
                                              double lat_deg, double lon_deg,
                                              double alt_m);
neb_status_t neb_rtk_standalone_enable_time(neb_handle_t *handle,
                                            unsigned seconds);

// SBAS enable/disable (Manual §4.10). Applicable to UM960/UM980/UM982; the
// UM960L returns NEB_ERR_UNSUPPORTED.
neb_status_t neb_rtk_sbas_enable(neb_handle_t *handle,
                                 neb_sbas_system_t system);
neb_status_t neb_rtk_sbas_disable(neb_handle_t *handle);

// SBAS timeout (Manual §4.10). Applicable to UM982 only; other models return
// NEB_ERR_UNSUPPORTED.
//
// Build-gated: requires UM982 Build9669+. The capability check is model-level
// only; an older build replies NEB_ERR_NAK.
neb_status_t neb_rtk_sbas_timeout(neb_handle_t *handle, unsigned seconds);

// Base-station antenna model (Manual §4.26). Available on every model.
neb_status_t neb_rtk_base_antenna(neb_handle_t *handle, const char *name,
                                  const char *sn, unsigned setup_id,
                                  neb_antenna_type_t type);

// Base-station antenna height / plane offset (Manual §4.22). Available on every
// model.
neb_status_t neb_rtk_antenna_delta(neb_handle_t *handle, double height_m,
                                   double east_m, double north_m);

#endif
