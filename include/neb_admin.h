// neb_admin.h
//
// "Other" administrative commands (Manual §8): stop message output (UNLOG),
// restart / clear the receiver (RESET), factory-reset (FRESET), and persist the
// configuration (SAVECONFIG). All are top-level commands on every model
// (NEB_CAP_ADMIN).
//
// UNLOG is benign and reversible. FRESET, RESET, and SAVECONFIG are disruptive
// -- see the per-function warnings. None is validated on hardware here because
// there is no safe value to send.
#ifndef NEB_ADMIN_H
#define NEB_ADMIN_H

#include <stddef.h>
#include <stdint.h>

#include "neb_core.h"
#include "neb_protocol.h" // neb_com_port_t

// Targets to clear on RESET (Manual §8.3, Table 8-3). Combine with '|'.
typedef uint32_t neb_reset_flags_t;
#define NEB_RESET_EPHEM (1u << 0)    // ephemeris
#define NEB_RESET_ALMANAC (1u << 1)  // almanac
#define NEB_RESET_IONUTC (1u << 2)   // ionosphere & UTC parameters
#define NEB_RESET_POSITION (1u << 3) // saved position
#define NEB_RESET_XOPARAM                                                      \
  (1u << 4) // crystal oscillator info (a.k.a CLOCKDRIFT)

// --- Builders (pure; no handle, no transport) -----------------------------

// Stop message output (Manual §8.1). Four forms: all messages on the current
// port; a specific message on the current port; all messages on a given port;
// a specific message on a given port. `message` is a log name (e.g. "GPGGA");
// it must be 1..31 printable non-space ASCII characters.
neb_status_t neb_build_admin_unlog_all(char *out, size_t len);
neb_status_t neb_build_admin_unlog_message(char *out, size_t len,
                                           const char *message);
neb_status_t neb_build_admin_unlog_port(char *out, size_t len,
                                        neb_com_port_t port);
neb_status_t neb_build_admin_unlog_port_message(char *out, size_t len,
                                                neb_com_port_t port,
                                                const char *message);

// Factory reset (Manual §8.2): clears all config, ephemerides, and position,
// resets the baud rate to 115200, and restarts the receiver.
neb_status_t neb_build_admin_freset(char *out, size_t len);

// Restart (Manual §8.3). Bare restart, restart clearing everything (ALL), or
// restart clearing a chosen combination of targets. reset_clear requires at
// least one flag (else NEB_ERR_INVALID_PARAM).
neb_status_t neb_build_admin_reset(char *out, size_t len);
neb_status_t neb_build_admin_reset_all(char *out, size_t len);
neb_status_t neb_build_admin_reset_clear(char *out, size_t len,
                                         neb_reset_flags_t flags);

// Save the current configuration to NVM (Manual §8.4).
neb_status_t neb_build_admin_saveconfig(char *out, size_t len);

// --- Wrappers (capability check + build + send) ---------------------------
// All gate on NEB_CAP_ADMIN (every model).

// Stop message output (Manual §8.1). Reversible (re-request the log); not
// persisted unless followed by SAVECONFIG.
neb_status_t neb_admin_unlog_all(neb_handle_t *handle);
neb_status_t neb_admin_unlog_message(neb_handle_t *handle, const char *message);
neb_status_t neb_admin_unlog_port(neb_handle_t *handle, neb_com_port_t port);
neb_status_t neb_admin_unlog_port_message(neb_handle_t *handle,
                                          neb_com_port_t port,
                                          const char *message);

// WARNING (Manual §8.2): FRESET erases ALL user configuration, ephemerides, and
// position from NVM, resets the serial baud rate to 115200 (you will lose the
// connection if you were at another rate), and restarts the receiver. This is a
// destructive factory reset -- there is no undo.
neb_status_t neb_admin_freset(neb_handle_t *handle);

// WARNING (Manual §8.3): RESET restarts the receiver (and, with flags/ALL,
// clears the selected saved data). It interrupts the current solution.
neb_status_t neb_admin_reset(neb_handle_t *handle);
neb_status_t neb_admin_reset_all(neb_handle_t *handle);
neb_status_t neb_admin_reset_clear(neb_handle_t *handle,
                                   neb_reset_flags_t flags);

// WARNING (Manual §8.4): SAVECONFIG persists the current configuration to NVM,
// making it survive a power cycle. Use deliberately -- it commits whatever
// configuration is currently active.
neb_status_t neb_admin_saveconfig(neb_handle_t *handle);

#endif
