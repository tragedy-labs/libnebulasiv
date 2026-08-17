// neb_mask.h
//
// MASK / UNMASK commands (Manual §5): choose which constellations, frequencies,
// and individual satellites the receiver tracks, and set the elevation mask
// angle. MASK/UNMASK are top-level commands (not CONFIG subcommands).
//
// Covers constellation-, frequency-, elevation-, and satellite-level masking,
// plus the UM982-only RTCMCN0/CN0 observation filters.
//
// Note (Manual §5.2): system-level and satellite-ID masking cannot be mixed --
// after MASK-ing a whole system you cannot UNMASK a specific satellite of it.
// The library does not enforce this cross-command ordering; it is the caller's
// responsibility.
#ifndef NEB_MASK_H
#define NEB_MASK_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_gnss_t, neb_gnss_freq_t

// --- Builders (pure; no handle, no transport) -----------------------------

// Query the current MASK configuration (Manual §5.1): the bare "MASK" command.
neb_status_t neb_build_mask_query(char *out, size_t len);

// Mask/unmask a whole constellation (Manual §5.2/§5.3), e.g. "MASK GPS".
neb_status_t neb_build_mask_gnss(char *out, size_t len, neb_gnss_t gnss);
neb_status_t neb_build_unmask_gnss(char *out, size_t len, neb_gnss_t gnss);

// Set the elevation mask angle in degrees (Manual §5.2), e.g. "MASK 10".
// Range -90..90; out of range returns NEB_ERR_INVALID_PARAM. (UNMASK has no
// elevation form.)
neb_status_t neb_build_mask_elevation(char *out, size_t len, int angle_deg);

// Set the elevation mask angle for one constellation (Manual §5.2), e.g.
// "MASK 10 GPS". Range -90..90.
neb_status_t neb_build_mask_elevation_gnss(char *out, size_t len, int angle_deg,
                                           neb_gnss_t gnss);

// Mask/unmask a single satellite by PRN (Manual §5.2/§5.3), e.g.
// "MASK GPS PRN 10". `prn` must be >= 1; the exact per-constellation valid
// range (manual Table 7-52) is enforced by the device.
neb_status_t neb_build_mask_satellite(char *out, size_t len, neb_gnss_t gnss,
                                      unsigned prn);
neb_status_t neb_build_unmask_satellite(char *out, size_t len, neb_gnss_t gnss,
                                        unsigned prn);

// Mask/unmask a single frequency (Manual §5.2/§5.3), e.g. "MASK B1".
neb_status_t neb_build_mask_frequency(char *out, size_t len,
                                      neb_gnss_freq_t freq);
neb_status_t neb_build_unmask_frequency(char *out, size_t len,
                                        neb_gnss_freq_t freq);

// Set the elevation mask angle for one frequency (Manual §5.2), e.g.
// "MASK 10 B1". Range -90..90.
neb_status_t neb_build_mask_elevation_frequency(char *out, size_t len,
                                                int angle_deg,
                                                neb_gnss_freq_t freq);

// C/N0 masking (Manual §5.2, Table 5-5). RTCMCN0 limits RTCM observation
// output; CN0 limits OBSV observation output. The `_freq` variants restrict to
// one frequency; the plain variants apply to all frequencies. The manual gives
// no numeric range for the C/N0 value, so it is passed through as-is.
neb_status_t neb_build_mask_rtcmcn0(char *out, size_t len, unsigned cn0);
neb_status_t neb_build_mask_rtcmcn0_frequency(char *out, size_t len,
                                              unsigned cn0,
                                              neb_gnss_freq_t freq);
neb_status_t neb_build_mask_cn0(char *out, size_t len, unsigned cn0);
neb_status_t neb_build_mask_cn0_frequency(char *out, size_t len, unsigned cn0,
                                          neb_gnss_freq_t freq);

// --- Wrappers (capability check + build + send) ---------------------------
// MASK/UNMASK are available on every model (gate on NEB_CAP_MASK).

// On success `response` receives the raw "$CONFIG,MASK,..." lines; pass NULL if
// not needed.
neb_status_t neb_mask_query(neb_handle_t *handle, char *response,
                            size_t response_size);

neb_status_t neb_mask_gnss(neb_handle_t *handle, neb_gnss_t gnss);
neb_status_t neb_unmask_gnss(neb_handle_t *handle, neb_gnss_t gnss);
neb_status_t neb_mask_elevation(neb_handle_t *handle, int angle_deg);
neb_status_t neb_mask_elevation_gnss(neb_handle_t *handle, int angle_deg,
                                     neb_gnss_t gnss);
neb_status_t neb_mask_satellite(neb_handle_t *handle, neb_gnss_t gnss,
                                unsigned prn);
neb_status_t neb_unmask_satellite(neb_handle_t *handle, neb_gnss_t gnss,
                                  unsigned prn);
neb_status_t neb_mask_frequency(neb_handle_t *handle, neb_gnss_freq_t freq);
neb_status_t neb_unmask_frequency(neb_handle_t *handle, neb_gnss_freq_t freq);
neb_status_t neb_mask_elevation_frequency(neb_handle_t *handle, int angle_deg,
                                          neb_gnss_freq_t freq);

// C/N0 masking (Manual §5.2, Table 5-5). Applicable to UM982 only; other models
// return NEB_ERR_UNSUPPORTED.
//
// Build-gated: requires UM982 Build9669+. The capability check is model-level
// only; an older build replies NEB_ERR_NAK.
neb_status_t neb_mask_rtcmcn0(neb_handle_t *handle, unsigned cn0);
neb_status_t neb_mask_rtcmcn0_frequency(neb_handle_t *handle, unsigned cn0,
                                        neb_gnss_freq_t freq);
neb_status_t neb_mask_cn0(neb_handle_t *handle, unsigned cn0);
neb_status_t neb_mask_cn0_frequency(neb_handle_t *handle, unsigned cn0,
                                    neb_gnss_freq_t freq);

#endif
