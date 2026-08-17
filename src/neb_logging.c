#include "neb_logging.h"
#include "neb_internal.h" // neb_valid_token
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O.
// ===========================================================================

// A log-message name is a single unquoted wire token; validate with
// neb_valid_token(..., 0) (no embedded spaces).

// Manual §7 -- <message> (output once, current port)
neb_status_t neb_build_logging_once(char *out, size_t len,
                                    const char *message) {
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s", message);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §7 -- <message> <port> (output once)
neb_status_t neb_build_logging_once_port(char *out, size_t len,
                                         const char *message,
                                         neb_com_port_t port) {
  const char *port_str = neb_com_port_str(port);
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s", message, port_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §7 -- <message> <period> (current port)
neb_status_t neb_build_logging_periodic(char *out, size_t len,
                                        const char *message, double period_s) {
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;
  if (!(period_s > 0.0))
    return NEB_ERR_INVALID_PARAM;

  // "GPGGA 1" / "GPGGA 0.5"
  int n = snprintf(out, len, "%s %g", message, period_s);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §7 -- <message> <port> <period>
neb_status_t neb_build_logging_periodic_port(char *out, size_t len,
                                             const char *message,
                                             neb_com_port_t port,
                                             double period_s) {
  const char *port_str = neb_com_port_str(port);
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;
  if (!(period_s > 0.0))
    return NEB_ERR_INVALID_PARAM;

  // "GPGGA COM2 1"
  int n = snprintf(out, len, "%s %s %g", message, port_str, period_s);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §7 -- <message> ONCHANGED (current port)
neb_status_t neb_build_logging_onchanged(char *out, size_t len,
                                         const char *message) {
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s", message, NEB_TOK_ONCHANGED);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §7 -- <message> <port> ONCHANGED
neb_status_t neb_build_logging_onchanged_port(char *out, size_t len,
                                              const char *message,
                                              neb_com_port_t port) {
  const char *port_str = neb_com_port_str(port);
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s %s", message, port_str, NEB_TOK_ONCHANGED);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send. All gate on NEB_CAP_LOGGING.
// ===========================================================================

neb_status_t neb_logging_once(neb_handle_t *handle, const char *message) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_logging_once(cmd, sizeof(cmd), message);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_logging_once_port(neb_handle_t *handle, const char *message,
                                   neb_com_port_t port) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_logging_once_port(cmd, sizeof(cmd), message, port);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_logging_periodic(neb_handle_t *handle, const char *message,
                                  double period_s) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_logging_periodic(cmd, sizeof(cmd), message, period_s);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_logging_periodic_port(neb_handle_t *handle,
                                       const char *message, neb_com_port_t port,
                                       double period_s) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_logging_periodic_port(cmd, sizeof(cmd), message,
                                                    port, period_s);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_logging_onchanged(neb_handle_t *handle, const char *message) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_logging_onchanged(cmd, sizeof(cmd), message);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_logging_onchanged_port(neb_handle_t *handle,
                                        const char *message,
                                        neb_com_port_t port) {
  if (!neb_has_cap(handle, NEB_CAP_LOGGING))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_logging_onchanged_port(cmd, sizeof(cmd), message, port);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

// ===========================================================================
// Typed NMEA wrappers -- resolve the enum to its wire name, then delegate to
// the string variants above (which do the capability check and send).
// ===========================================================================

neb_status_t neb_logging_nmea_once(neb_handle_t *handle,
                                   neb_nmea_message_t msg) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_once(handle, name);
}

neb_status_t neb_logging_nmea_once_port(neb_handle_t *handle,
                                        neb_nmea_message_t msg,
                                        neb_com_port_t port) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_once_port(handle, name, port);
}

neb_status_t neb_logging_nmea_periodic(neb_handle_t *handle,
                                       neb_nmea_message_t msg,
                                       double period_s) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_periodic(handle, name, period_s);
}

neb_status_t neb_logging_nmea_periodic_port(neb_handle_t *handle,
                                            neb_nmea_message_t msg,
                                            neb_com_port_t port,
                                            double period_s) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_periodic_port(handle, name, port, period_s);
}

neb_status_t neb_logging_nmea_onchanged(neb_handle_t *handle,
                                        neb_nmea_message_t msg) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_onchanged(handle, name);
}

neb_status_t neb_logging_nmea_onchanged_port(neb_handle_t *handle,
                                             neb_nmea_message_t msg,
                                             neb_com_port_t port) {
  const char *name = neb_nmea_message_str(msg);
  if (!name)
    return NEB_ERR_INVALID_PARAM;
  return neb_logging_onchanged_port(handle, name, port);
}
