// neb_config.h
//
// CONFIG-family commands (Manual §4): receiver function/interface
// configuration. This module holds the general CONFIG commands shared across
// the product line; capability differences are enforced per-command via the
// handle's capability flags.
//
// Each command is split into a pure `neb_build_*` function (formats the exact
// wire string, validates parameters, no I/O -- unit-testable on its own) and a
// thin `neb_config_*` wrapper (capability check, build, send).
#ifndef NEB_CONFIG_H
#define NEB_CONFIG_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_ppp_mode_t

// --- Builders (pure; no handle, no transport) -----------------------------
// Each writes a NUL-terminated command into `out` and returns NEB_OK, or
// NEB_ERR_INVALID_PARAM for a bad parameter / NEB_ERR_OVERFLOW if it would not
// fit in `len`.

neb_status_t neb_build_config_query(char *out, size_t len);
neb_status_t neb_build_config_ppp_enable(char *out, size_t len,
                                         neb_ppp_mode_t mode);
neb_status_t neb_build_config_ppp_disable(char *out, size_t len);
neb_status_t neb_build_config_ppp_converge(char *out, size_t len,
                                           double hor_std_cm,
                                           double ver_std_cm);
neb_status_t neb_build_config_serial(char *out, size_t len, neb_com_port_t port,
                                     unsigned baud);
neb_status_t neb_build_config_nmea_version(char *out, size_t len,
                                           neb_nmea_version_t version);
neb_status_t neb_build_config_antijam(char *out, size_t len,
                                      neb_antijam_mode_t mode);
neb_status_t neb_build_config_agnss_enable(char *out, size_t len);
neb_status_t neb_build_config_agnss_disable(char *out, size_t len);
neb_status_t neb_build_config_undulation_auto(char *out, size_t len);
neb_status_t neb_build_config_undulation(char *out, size_t len,
                                         double separation_m);
neb_status_t neb_build_config_smooth_rtkheight(char *out, size_t len,
                                               unsigned epochs);
neb_status_t neb_build_config_smooth_heading(char *out, size_t len,
                                             unsigned epochs);
neb_status_t neb_build_config_smooth_psrvel_enable(char *out, size_t len);
neb_status_t neb_build_config_smooth_psrvel_disable(char *out, size_t len);
neb_status_t neb_build_config_rtcm_b1c_b2a_enable(char *out, size_t len);
neb_status_t neb_build_config_rtcm_b1c_b2a_disable(char *out, size_t len);
neb_status_t neb_build_config_ionmode(char *out, size_t len,
                                      neb_ionmode_t mode);
neb_status_t neb_build_config_mmp_enable(char *out, size_t len);
neb_status_t neb_build_config_mmp_disable(char *out, size_t len);
neb_status_t neb_build_config_event_disable(char *out, size_t len);
neb_status_t neb_build_config_event_enable(char *out, size_t len,
                                           neb_polarity_t polarity,
                                           unsigned tguard_ms);
neb_status_t neb_build_config_rtcmphaserate_positive(char *out, size_t len);
neb_status_t neb_build_config_rtcmphaserate_negative(char *out, size_t len);
neb_status_t neb_build_config_psrveldrpos_enable(char *out, size_t len);
neb_status_t neb_build_config_psrveldrpos_disable(char *out, size_t len);
neb_status_t neb_build_config_pps_disable(char *out, size_t len);
neb_status_t neb_build_config_pps_enable(char *out, size_t len,
                                         neb_pps_mode_t mode,
                                         neb_pps_timeref_t timeref,
                                         neb_polarity_t polarity,
                                         unsigned width_us, unsigned period_ms,
                                         int rf_delay_ns, int user_delay_ns);
neb_status_t neb_build_config_signalgroup(char *out, size_t len,
                                          unsigned master);
neb_status_t neb_build_config_signalgroup_dual(char *out, size_t len,
                                               unsigned master, unsigned slave);
neb_status_t neb_build_config_algreset(char *out, size_t len,
                                       neb_algreset_type_t type);

// --- Wrappers (capability check + build + send) ---------------------------

// Query the full receiver configuration (Manual §4.1). On success `response`
// receives the raw "$CONFIG,...*hh" lines; pass NULL if not needed.
neb_status_t neb_config_query(neb_handle_t *handle, char *response,
                              size_t response_size);

// Enable PPP with the given correction source (Manual §4.19). Applicable to
// UM980 and UM982 only; other models return NEB_ERR_UNSUPPORTED.
neb_status_t neb_config_ppp_enable(neb_handle_t *handle, neb_ppp_mode_t mode);

// Disable PPP (Manual §4.19). UM980/UM982 only.
neb_status_t neb_config_ppp_disable(neb_handle_t *handle);

// Set the PPP convergence thresholds in centimetres (Manual §4.19, Table 4-25).
// UM980/UM982 only. The manual gives no numeric range, so only non-positive
// values are rejected (a standard-deviation threshold must be > 0), returning
// NEB_ERR_INVALID_PARAM without touching the wire.
neb_status_t neb_config_ppp_converge(neb_handle_t *handle, double hor_std_cm,
                                     double ver_std_cm);

// Set a serial port's baud rate (Manual §4.2). `baud` must be one of the
// documented rates (9600, 19200, 38400, 57600, 115200, 230400, 460800,
// 921600); otherwise NEB_ERR_INVALID_PARAM. Note you can configure the port you
// are talking through, which will change the rate mid-session.
neb_status_t neb_config_serial(neb_handle_t *handle, neb_com_port_t port,
                               unsigned baud);

// Select the NMEA 0183 output version (Manual §4.14).
neb_status_t neb_config_nmea_version(neb_handle_t *handle,
                                     neb_nmea_version_t version);

// Set the anti-jamming mode (Manual §4.20). Applicable to UM960 and UM980 only;
// other models return NEB_ERR_UNSUPPORTED.
//
// Build-gated: on UM980, ANTIJAM requires Build7923+. The capability check is
// model-level only; an older build replies NEB_ERR_NAK.
neb_status_t neb_config_antijam(neb_handle_t *handle, neb_antijam_mode_t mode);

// Enable / disable AGNSS, which shortens time-to-first-fix (Manual §4.18).
// Applicable to UM960, UM980, UM982; the UM960L returns NEB_ERR_UNSUPPORTED.
//
// Build-gated: UM980 requires Build7923+, UM982 requires Build7650+. The
// capability check is model-level only; an older build replies NEB_ERR_NAK.
neb_status_t neb_config_agnss_enable(neb_handle_t *handle);
neb_status_t neb_config_agnss_disable(neb_handle_t *handle);

// Use the receiver's built-in geoid undulation grid (Manual §4.4).
neb_status_t neb_config_undulation_auto(neb_handle_t *handle);

// Set a user-specified geoid undulation in metres (Manual §4.4, Table 4-7).
// Valid range -1000.0000..+1000.0000 m; out of range returns
// NEB_ERR_INVALID_PARAM. Configure this before base-station mode.
neb_status_t neb_config_undulation(neb_handle_t *handle, double separation_m);

// Smoothing of RTK height / heading over `epochs` (Manual §4.12). Valid range
// 0..100 epochs; out of range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_config_smooth_rtkheight(neb_handle_t *handle, unsigned epochs);
neb_status_t neb_config_smooth_heading(neb_handle_t *handle, unsigned epochs);

// Enable / disable smoothing of Doppler velocity in SPPNAV (Manual §4.12).
neb_status_t neb_config_smooth_psrvel_enable(neb_handle_t *handle);
neb_status_t neb_config_smooth_psrvel_disable(neb_handle_t *handle);

// Include / exclude BDS B1C & B2a signals in the RTCM stream (Manual §4.15).
// Applicable to UM980/UM982 only; other models return NEB_ERR_UNSUPPORTED.
neb_status_t neb_config_rtcm_b1c_b2a_enable(neb_handle_t *handle);
neb_status_t neb_config_rtcm_b1c_b2a_disable(neb_handle_t *handle);

// Select the ionospheric model (Manual §4.25). Applicable to UM960/UM980/UM982;
// the UM960L returns NEB_ERR_UNSUPPORTED.
//
// Build-gated: UM982 requires Build9669+. The capability check is model-level
// only; an older build replies NEB_ERR_NAK. Note the manual marks BD2K8 and
// BD3GIM "not supported currently" -- the device may NAK those regardless.
neb_status_t neb_config_ionmode(neb_handle_t *handle, neb_ionmode_t mode);

// Enable / disable multi-path mitigation (Manual §4.13). Applicable to UM980
// only; other models return NEB_ERR_UNSUPPORTED.
neb_status_t neb_config_mmp_enable(neb_handle_t *handle);
neb_status_t neb_config_mmp_disable(neb_handle_t *handle);

// Disable the EVENT input, or enable it with a trigger edge and a TGUARD
// debounce in milliseconds (Manual §4.11). TGUARD is 2..3599999 ms (default 4);
// out of range returns NEB_ERR_INVALID_PARAM. Applicable to UM960/UM980/UM982;
// the UM960L returns NEB_ERR_UNSUPPORTED.
neb_status_t neb_config_event_disable(neb_handle_t *handle);
neb_status_t neb_config_event_enable(neb_handle_t *handle,
                                     neb_polarity_t polarity,
                                     unsigned tguard_ms);

// Set the sign of the Phaserange Rate in RTCM MSM5/MSM7 messages (Manual
// §4.16). Applicable to UM980/UM982; other models return NEB_ERR_UNSUPPORTED.
//
// Build-gated: UM982 requires Build9669+. The capability check is model-level
// only; an older build replies NEB_ERR_NAK.
neb_status_t neb_config_rtcmphaserate_positive(neb_handle_t *handle);
neb_status_t neb_config_rtcmphaserate_negative(neb_handle_t *handle);

// Enable / disable Doppler position prediction (Manual §4.17). Applicable to
// UM980 only; other models return NEB_ERR_UNSUPPORTED.
neb_status_t neb_config_psrveldrpos_enable(neb_handle_t *handle);
neb_status_t neb_config_psrveldrpos_disable(neb_handle_t *handle);

// Disable the PPS output (Manual §4.3). All models.
neb_status_t neb_config_pps_disable(neb_handle_t *handle);

// Enable the PPS output with full pulse parameters (Manual §4.3, Tables 4-5/6).
// All models support ENABLE; ENABLE2 and ENABLE3 are not available on the
// UM960L (returns NEB_ERR_UNSUPPORTED). Documented parameter bounds, validated
// before send (else NEB_ERR_INVALID_PARAM):
//   width_us     pulse width, microseconds; must be < period (period_ms*1000)
//   period_ms    output period, 50..20000 ms (the exact allowed set beyond the
//                endpoints is underspecified in the manual; the device is the
//                final arbiter of intermediate values)
//   rf_delay_ns  RF delay, -32768..32767 ns
//   user_delay_ns user-set delay, -32768..32767 ns
neb_status_t neb_config_pps_enable(neb_handle_t *handle, neb_pps_mode_t mode,
                                   neb_pps_timeref_t timeref,
                                   neb_polarity_t polarity, unsigned width_us,
                                   unsigned period_ms, int rf_delay_ns,
                                   int user_delay_ns);

// Signal group -- signals tracked by the master (`master`) and, on the dual
// form, the slave (`slave`) antenna (Manual §4.21, Table 4-28). TypeNum values
// are 0..7. Applicable to UM980/UM982; other models return NEB_ERR_UNSUPPORTED.
//
// The dual (slave) form requires a dual-antenna receiver (UM982); a
// single-antenna model returns a device error (NEB_ERR_NAK). NOTE: this command
// auto-saves and resets the module -- it takes effect immediately and persists
// (no SAVECONFIG needed). TypeNum 0 ("disable slave antenna") is documented for
// the slave parameter; its meaning as the master parameter is unclear, so the
// full 0..7 range is accepted and the device is the final arbiter.
neb_status_t neb_config_signalgroup(neb_handle_t *handle, unsigned master);
neb_status_t neb_config_signalgroup_dual(neb_handle_t *handle, unsigned master,
                                         unsigned slave);

// Reset a receiver algorithm (Manual §4.24). Applicable to UM960/UM980/UM982,
// but individual types are further restricted: RTK2 and HEADING require a
// UM982, PPP requires UM980/UM982 -- an unsupported type on the current model
// returns NEB_ERR_UNSUPPORTED. This disrupts the affected solution.
neb_status_t neb_config_algreset(neb_handle_t *handle,
                                 neb_algreset_type_t type);

#endif
