// neb_heading.h
//
// Dual-antenna heading configuration (Manual §4.8-§4.9): baseline behavior,
// heading reliability, baseline length, and heading/pitch angle offsets.
//
// ===========================================================================
// Hardware status: the §4.8 commands (heading mode / reliability / length,
// gated on NEB_CAP_HEADING) are VERIFIED on a Holybro UM982 (R4.10Build11826) --
// each is accepted by the device. The one exception is
// neb_(build_)heading_length_default (bare "CONFIG HEADING LENGTH"): the manual
// documents it (Table 4-13 defaults) but that firmware REJECTS it with a
// GRAMMAR ERROR -- prefer the parameterized neb_heading_length. See its note
// below and HARDWARE_TESTING.md.
//
// CONFIG HEADING OFFSET (§4.9) is UM980 + UM982 and hardware-checkable.
// ===========================================================================
#ifndef NEB_HEADING_H
#define NEB_HEADING_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_heading2_mode_t (shared baseline modes)

// --- Builders (pure; no handle, no transport) -----------------------------

// Set the baseline behavior mode (Manual §4.8, Table 4-12; verified on UM982
// R4.10Build11826). Reuses the MODE HEADING2 baseline enum -- §3.7 and §4.8
// share the same FIXLENGTH/VARIABLELENGTH/STATIC/LOWDYNAMIC/TRACTOR tokens.
neb_status_t neb_build_heading_mode(char *out, size_t len,
                                    neb_heading2_mode_t mode);

// Heading reliability threshold 1..4 (default 3) (Manual §4.8; verified on UM982
// R4.10Build11826). Out of range returns NEB_ERR_INVALID_PARAM.
neb_status_t neb_build_heading_reliability(char *out, size_t len,
                                           unsigned threshold);

// Baseline length using the default configuration -- bare "CONFIG HEADING
// LENGTH" (Manual §4.8, Table 4-13, which documents omitting both params to use
// defaults). HARDWARE NOTE: Holybro UM982 R4.10Build11826 REJECTS the bare form
// with "PARSING FAILD GRAMMAR ERROR" despite the manual; on that firmware use
// the parameterized neb_build_heading_length below instead.
neb_status_t neb_build_heading_length_default(char *out, size_t len);

// Baseline length and error tolerance, both in centimetres (Manual §4.8,
// Table 4-13; verified on UM982 R4.10Build11826). The manual gives no numeric
// bounds, so values are passed through (length must be >= 1 cm).
neb_status_t neb_build_heading_length(char *out, size_t len, unsigned length_cm,
                                      unsigned tolerance_cm);

// Heading and pitch angle offset corrections in degrees (Manual §4.9,
// Table 4-14). heading -180..180, pitch -90..90; out of range returns
// NEB_ERR_INVALID_PARAM. Applicable to UM980 and UM982 (hardware-checkable).
neb_status_t neb_build_heading_offset(char *out, size_t len, double heading_deg,
                                      double pitch_deg);

// --- Wrappers (capability check + build + send) ---------------------------

// §4.8 -- gate on NEB_CAP_HEADING. Verified on UM982 R4.10Build11826, except
// neb_heading_length_default (see the builder note above -- that firmware
// rejects the bare "CONFIG HEADING LENGTH" form; use neb_heading_length).
neb_status_t neb_heading_mode(neb_handle_t *handle, neb_heading2_mode_t mode);
neb_status_t neb_heading_reliability(neb_handle_t *handle, unsigned threshold);
neb_status_t neb_heading_length_default(neb_handle_t *handle);
neb_status_t neb_heading_length(neb_handle_t *handle, unsigned length_cm,
                                unsigned tolerance_cm);

// Heading/pitch offset (Manual §4.9). Applicable to UM980/UM982; other models
// return NEB_ERR_UNSUPPORTED. Not alpha -- verifiable on a UM980.
neb_status_t neb_heading_offset(neb_handle_t *handle, double heading_deg,
                                double pitch_deg);

#endif
