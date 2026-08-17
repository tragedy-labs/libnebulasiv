#include "neb_admin.h"
#include "neb_internal.h" // neb_valid_token
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O.
// ===========================================================================

// A log-message name is a single unquoted wire token; validate with
// neb_valid_token(..., 0) (no embedded spaces).

// Manual §8.1 -- UNLOG (current port, all messages)
neb_status_t neb_build_admin_unlog_all(char *out, size_t len) {
  int n = snprintf(out, len, "%s", NEB_CMD_UNLOG);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.1 -- UNLOG <message> (current port)
neb_status_t neb_build_admin_unlog_message(char *out, size_t len,
                                           const char *message) {
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s", NEB_CMD_UNLOG, message);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.1 -- UNLOG <port> (all messages)
neb_status_t neb_build_admin_unlog_port(char *out, size_t len,
                                        neb_com_port_t port) {
  const char *port_str = neb_com_port_str(port);
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s", NEB_CMD_UNLOG, port_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.1 -- UNLOG <port> <message>
neb_status_t neb_build_admin_unlog_port_message(char *out, size_t len,
                                                neb_com_port_t port,
                                                const char *message) {
  const char *port_str = neb_com_port_str(port);
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;
  if (!neb_valid_token(message, 0))
    return NEB_ERR_INVALID_PARAM;

  int n = snprintf(out, len, "%s %s %s", NEB_CMD_UNLOG, port_str, message);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.2 -- FRESET
neb_status_t neb_build_admin_freset(char *out, size_t len) {
  int n = snprintf(out, len, "%s", NEB_CMD_FRESET);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.3 -- RESET (bare restart)
neb_status_t neb_build_admin_reset(char *out, size_t len) {
  int n = snprintf(out, len, "%s", NEB_CMD_RESET);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.3 -- RESET ALL
neb_status_t neb_build_admin_reset_all(char *out, size_t len) {
  int n = snprintf(out, len, "%s %s", NEB_CMD_RESET, NEB_TOK_ALL);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §8.3 -- RESET <targets...>. Tokens are appended in the fixed order of
// the manual's multi-parameter example (EPHEM ALMANAC IONUTC POSITION XOPARAM).
neb_status_t neb_build_admin_reset_clear(char *out, size_t len,
                                         neb_reset_flags_t flags) {
  const neb_reset_flags_t known = NEB_RESET_EPHEM | NEB_RESET_ALMANAC |
                                  NEB_RESET_IONUTC | NEB_RESET_POSITION |
                                  NEB_RESET_XOPARAM;
  if (flags == 0 || (flags & ~known) != 0)
    return NEB_ERR_INVALID_PARAM;

  // Start with "RESET" then append each selected target.
  size_t pos = 0;
  int n = snprintf(out, len, "%s", NEB_CMD_RESET);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  pos = (size_t)n;

  static const struct {
    neb_reset_flags_t bit;
    const char *token;
  } order[] = {
      {NEB_RESET_EPHEM, NEB_TOK_EPHEM},
      {NEB_RESET_ALMANAC, NEB_TOK_ALMANAC},
      {NEB_RESET_IONUTC, NEB_TOK_IONUTC},
      {NEB_RESET_POSITION, NEB_TOK_POSITION},
      {NEB_RESET_XOPARAM, NEB_TOK_XOPARAM},
  };
  for (size_t i = 0; i < sizeof(order) / sizeof(order[0]); i++) {
    if (!(flags & order[i].bit))
      continue;
    n = snprintf(out + pos, len - pos, " %s", order[i].token);
    if (n < 0 || (size_t)n >= len - pos)
      return NEB_ERR_OVERFLOW;
    pos += (size_t)n;
  }
  return NEB_OK;
}

// Manual §8.4 -- SAVECONFIG
neb_status_t neb_build_admin_saveconfig(char *out, size_t len) {
  int n = snprintf(out, len, "%s", NEB_CMD_SAVECONFIG);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send. All gate on NEB_CAP_ADMIN.
// ===========================================================================

neb_status_t neb_admin_unlog_all(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_unlog_all(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_unlog_message(neb_handle_t *handle,
                                     const char *message) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_unlog_message(cmd, sizeof(cmd), message);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_unlog_port(neb_handle_t *handle, neb_com_port_t port) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_unlog_port(cmd, sizeof(cmd), port);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_unlog_port_message(neb_handle_t *handle,
                                          neb_com_port_t port,
                                          const char *message) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_admin_unlog_port_message(cmd, sizeof(cmd), port, message);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_freset(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_freset(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_reset(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_reset(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_reset_all(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_reset_all(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_reset_clear(neb_handle_t *handle,
                                   neb_reset_flags_t flags) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_reset_clear(cmd, sizeof(cmd), flags);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_admin_saveconfig(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_ADMIN))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_admin_saveconfig(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
