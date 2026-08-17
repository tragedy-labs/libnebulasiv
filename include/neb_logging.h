// neb_logging.h
//
// Data-output commands (Manual §7): request a message to be output once, at a
// periodic rate, or whenever it changes (ONCHANGED), optionally on a specific
// serial port. The uniform wire form is:
//
//     <message> [port] [period_seconds | ONCHANGED]
//
// with no period meaning "output once" and no port meaning "the current port".
//
// Two layers:
//   1. Generic string API (neb_logging_*): the caller names any message
//      (e.g. "GPGGA", "VERSIONA", "BESTNAVA"). Use this for messages not in the
//      typed enum, notably the Unicore ASCII/binary logs (§7.3).
//   2. Typed NMEA API (neb_logging_nmea_*): takes a neb_nmea_message_t
//      (Manual §7.1 standard NMEA + §7.2 slave-antenna variants) -- compile-
//      checked, no typos. See neb_nmea_message_t in neb_protocol.h.
//
// Per-message applicability is still the device's to enforce (an unsupported
// message -> NEB_ERR_NAK); the enum removes typos, not applicability surprises.
// To STOP a message, use the UNLOG commands in neb_admin.h.
//
// STILL DEFERRED in §7 (where we left off, for the eventual parser work):
//   - A typed enum for the §7.3 Unicore ASCII/binary logs (~80 messages:
//     VERSIONA, BESTNAVA, OBSVA, ...); today those go through the string API.
//   - Field-level PARSING of any output message into structs -- none of the §7
//     output payloads are decoded here. This module only turns messages on/off;
//     reading and interpreting the resulting stream is a separate future body
//     of work.
#ifndef NEB_LOGGING_H
#define NEB_LOGGING_H

#include <stddef.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_com_port_t

// `message` must be 1..31 printable non-space ASCII characters (a single log
// name); otherwise NEB_ERR_INVALID_PARAM. `period_s` is the output period in
// seconds and must be > 0 (e.g. 1 = 1 Hz, 0.5 = 2 Hz, 0.2 = 5 Hz, 0.1 = 10 Hz).

// --- Builders (pure; no handle, no transport) -----------------------------

// Output the message once (Manual §7), on the current port or a given port.
neb_status_t neb_build_logging_once(char *out, size_t len, const char *message);
neb_status_t neb_build_logging_once_port(char *out, size_t len,
                                         const char *message,
                                         neb_com_port_t port);

// Output the message periodically at `period_s` seconds, current/given port.
neb_status_t neb_build_logging_periodic(char *out, size_t len,
                                        const char *message, double period_s);
neb_status_t neb_build_logging_periodic_port(char *out, size_t len,
                                             const char *message,
                                             neb_com_port_t port,
                                             double period_s);

// Output the message whenever it changes (Manual §7, ONCHANGED). Only applies
// to particular Unicore-format messages; the device rejects it otherwise.
neb_status_t neb_build_logging_onchanged(char *out, size_t len,
                                         const char *message);
neb_status_t neb_build_logging_onchanged_port(char *out, size_t len,
                                              const char *message,
                                              neb_com_port_t port);

// --- Wrappers (capability check + build + send) ---------------------------
// All gate on NEB_CAP_LOGGING (every model). Per-message support is the
// device's to enforce.

neb_status_t neb_logging_once(neb_handle_t *handle, const char *message);
neb_status_t neb_logging_once_port(neb_handle_t *handle, const char *message,
                                   neb_com_port_t port);
neb_status_t neb_logging_periodic(neb_handle_t *handle, const char *message,
                                  double period_s);
neb_status_t neb_logging_periodic_port(neb_handle_t *handle,
                                       const char *message, neb_com_port_t port,
                                       double period_s);
neb_status_t neb_logging_onchanged(neb_handle_t *handle, const char *message);
neb_status_t neb_logging_onchanged_port(neb_handle_t *handle,
                                        const char *message,
                                        neb_com_port_t port);

// --- Typed convenience wrappers for the standard NMEA messages -------------
// Same behavior as the string variants above, but take a neb_nmea_message_t
// (Manual §7.1/§7.2) instead of a raw name -- compile-checked, autocompleted,
// no typos. An out-of-range enum returns NEB_ERR_INVALID_PARAM. For messages
// not in the enum (e.g. Unicore binary logs), use the string variants.
neb_status_t neb_logging_nmea_once(neb_handle_t *handle,
                                   neb_nmea_message_t msg);
neb_status_t neb_logging_nmea_once_port(neb_handle_t *handle,
                                        neb_nmea_message_t msg,
                                        neb_com_port_t port);
neb_status_t neb_logging_nmea_periodic(neb_handle_t *handle,
                                       neb_nmea_message_t msg, double period_s);
neb_status_t neb_logging_nmea_periodic_port(neb_handle_t *handle,
                                            neb_nmea_message_t msg,
                                            neb_com_port_t port,
                                            double period_s);
neb_status_t neb_logging_nmea_onchanged(neb_handle_t *handle,
                                        neb_nmea_message_t msg);
neb_status_t neb_logging_nmea_onchanged_port(neb_handle_t *handle,
                                             neb_nmea_message_t msg,
                                             neb_com_port_t port);

#endif
