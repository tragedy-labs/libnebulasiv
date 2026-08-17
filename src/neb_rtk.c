#include "neb_rtk.h"
#include "neb_internal.h" // neb_valid_token
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O.
// ===========================================================================

// Manual §4.5, Table 4-8 -- CONFIG DGPS TIMEOUT <seconds>
neb_status_t neb_build_rtk_dgps_timeout(char *out, size_t len,
                                        unsigned seconds) {
  // 0 disables DGPS; 1..1800 s is the documented range.
  if (seconds > 1800)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG DGPS TIMEOUT 100"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_DGPS,
                   NEB_TOK_TIMEOUT, seconds);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.6, Table 4-9 -- CONFIG RTK TIMEOUT <seconds>
neb_status_t neb_build_rtk_timeout(char *out, size_t len, unsigned seconds) {
  // 0 disables RTK; 1..1800 s is the documented range.
  if (seconds > 1800)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG RTK TIMEOUT 60"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_RTK,
                   NEB_TOK_TIMEOUT, seconds);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.6, Table 4-10 -- CONFIG RTK RELIABILITY <engine> <adr>
neb_status_t neb_build_rtk_reliability(char *out, size_t len, unsigned engine,
                                       unsigned adr) {
  // Both thresholds are documented as 1..4 (ADR values 2 and 3 are reserved;
  // the device is the arbiter of whether a reserved value is accepted).
  if (engine < 1 || engine > 4 || adr < 1 || adr > 4)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG RTK RELIABILITY 3 1"
  int n = snprintf(out, len, "%s %s %s %u %u", NEB_CMD_CONFIG, NEB_TOK_RTK,
                   NEB_TOK_RELIABILITY, engine, adr);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.6 -- CONFIG RTK USER_DEFAULTS
neb_status_t neb_build_rtk_user_defaults(char *out, size_t len) {
  // "CONFIG RTK USER_DEFAULTS"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTK,
                   NEB_TOK_USER_DEFAULTS);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.6 -- CONFIG RTK RESET
neb_status_t neb_build_rtk_reset(char *out, size_t len) {
  // "CONFIG RTK RESET"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTK,
                   NEB_TOK_RESET);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.6 -- CONFIG RTK DISABLE
neb_status_t neb_build_rtk_disable(char *out, size_t len) {
  // "CONFIG RTK DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTK,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.7 -- CONFIG STANDALONE DISABLE
neb_status_t neb_build_rtk_standalone_disable(char *out, size_t len) {
  // "CONFIG STANDALONE DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_STANDALONE,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.7 -- CONFIG STANDALONE ENABLE (default, no parameters)
neb_status_t neb_build_rtk_standalone_enable(char *out, size_t len) {
  // "CONFIG STANDALONE ENABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_STANDALONE,
                   NEB_TOK_ENABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.7, Table 4-11 -- CONFIG STANDALONE ENABLE <lat> <lon> <alt>
neb_status_t neb_build_rtk_standalone_enable_coords(char *out, size_t len,
                                                    double lat_deg,
                                                    double lon_deg,
                                                    double alt_m) {
  // Documented bounds. Note the altitude ceiling is 18000 m here (Table 4-11),
  // not 30000 as in MODE BASE.
  if (lat_deg < -90.0 || lat_deg > 90.0)
    return NEB_ERR_INVALID_PARAM;
  if (lon_deg < -180.0 || lon_deg > 180.0)
    return NEB_ERR_INVALID_PARAM;
  if (alt_m < -30000.0 || alt_m > 18000.0)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG STANDALONE ENABLE 40.113452 114.212234 57.23". Fixed-point,
  // matching the MODE BASE precision policy (11 decimals for lat/lon, 4 for
  // altitude).
  int n =
      snprintf(out, len, "%s %s %s %.11f %.11f %.4f", NEB_CMD_CONFIG,
               NEB_TOK_STANDALONE, NEB_TOK_ENABLE_UC, lat_deg, lon_deg, alt_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.7, Table 4-11 -- CONFIG STANDALONE ENABLE <time>
neb_status_t neb_build_rtk_standalone_enable_time(char *out, size_t len,
                                                  unsigned seconds) {
  // Documented range: 3..100 s (default 100).
  if (seconds < 3 || seconds > 100)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG STANDALONE ENABLE 60"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_STANDALONE,
                   NEB_TOK_ENABLE_UC, seconds);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.10, Table 4-15 -- CONFIG SBAS ENABLE <system>
neb_status_t neb_build_rtk_sbas_enable(char *out, size_t len,
                                       neb_sbas_system_t system) {
  const char *system_str = neb_sbas_system_str(system);
  if (!system_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SBAS ENABLE WAAS"
  int n = snprintf(out, len, "%s %s %s %s", NEB_CMD_CONFIG, NEB_TOK_SBAS,
                   NEB_TOK_ENABLE_UC, system_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.10, Table 4-15 -- CONFIG SBAS DISABLE
neb_status_t neb_build_rtk_sbas_disable(char *out, size_t len) {
  // "CONFIG SBAS DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_SBAS,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.10, Table 4-15 -- CONFIG SBAS TIMEOUT <seconds>
neb_status_t neb_build_rtk_sbas_timeout(char *out, size_t len,
                                        unsigned seconds) {
  // Documented range: 120..1800 s (default 1200).
  if (seconds < 120 || seconds > 1800)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SBAS TIMEOUT 600"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_SBAS,
                   NEB_TOK_TIMEOUT, seconds);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.26, Table 4-34 -- CONFIG BASEANTENNAMODEL "<name>" <sn> <id> <type>
neb_status_t neb_build_rtk_base_antenna(char *out, size_t len, const char *name,
                                        const char *sn, unsigned setup_id,
                                        neb_antenna_type_t type) {
  const char *type_str = neb_antenna_type_str(type);
  if (!type_str)
    return NEB_ERR_INVALID_PARAM;
  // Name is quoted on the wire (spaces allowed); serial number is unquoted.
  if (!neb_valid_token(name, 1))
    return NEB_ERR_INVALID_PARAM;
  if (!neb_valid_token(sn, 0))
    return NEB_ERR_INVALID_PARAM;
  if (setup_id > 255)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG BASEANTENNAMODEL \"HXCCGX601A HXCS\" 62815 1 USER"
  int n = snprintf(out, len, "%s %s \"%s\" %s %u %s", NEB_CMD_CONFIG,
                   NEB_TOK_BASEANTENNAMODEL, name, sn, setup_id, type_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.22, Table 4-30 -- CONFIG ANTENNADELTAHEN <height> <east> <north>
neb_status_t neb_build_rtk_antenna_delta(char *out, size_t len, double height_m,
                                         double east_m, double north_m) {
  // Documented ranges, all metres.
  if (height_m < 0.0 || height_m > 6.5535)
    return NEB_ERR_INVALID_PARAM;
  if (east_m < 0.0 || east_m > 100.0)
    return NEB_ERR_INVALID_PARAM;
  if (north_m < 0.0 || north_m > 100.0)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG ANTENNADELTAHEN 1.5210 0.0000 0.0000" (4 decimals per the 0.1 mm
  // resolution implied by the 0.0000-6.5535 range).
  int n = snprintf(out, len, "%s %s %.4f %.4f %.4f", NEB_CMD_CONFIG,
                   NEB_TOK_ANTENNADELTAHEN, height_m, east_m, north_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send. All are CONFIG subcommands
// supported on every model, so they gate on NEB_CAP_CONFIG.
// ===========================================================================

neb_status_t neb_rtk_dgps_timeout(neb_handle_t *handle, unsigned seconds) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_dgps_timeout(cmd, sizeof(cmd), seconds);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_timeout(neb_handle_t *handle, unsigned seconds) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_timeout(cmd, sizeof(cmd), seconds);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_reliability(neb_handle_t *handle, unsigned engine,
                                 unsigned adr) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_reliability(cmd, sizeof(cmd), engine, adr);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_user_defaults(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_user_defaults(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_reset(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_reset(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_standalone_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_STANDALONE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_standalone_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_standalone_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_STANDALONE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_standalone_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_standalone_enable_coords(neb_handle_t *handle,
                                              double lat_deg, double lon_deg,
                                              double alt_m) {
  if (!neb_has_cap(handle, NEB_CAP_STANDALONE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_standalone_enable_coords(
      cmd, sizeof(cmd), lat_deg, lon_deg, alt_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_standalone_enable_time(neb_handle_t *handle,
                                            unsigned seconds) {
  if (!neb_has_cap(handle, NEB_CAP_STANDALONE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_rtk_standalone_enable_time(cmd, sizeof(cmd), seconds);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_sbas_enable(neb_handle_t *handle,
                                 neb_sbas_system_t system) {
  if (!neb_has_cap(handle, NEB_CAP_SBAS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_sbas_enable(cmd, sizeof(cmd), system);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_sbas_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_SBAS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_sbas_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_sbas_timeout(neb_handle_t *handle, unsigned seconds) {
  if (!neb_has_cap(handle, NEB_CAP_SBAS_TIMEOUT))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_rtk_sbas_timeout(cmd, sizeof(cmd), seconds);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_base_antenna(neb_handle_t *handle, const char *name,
                                  const char *sn, unsigned setup_id,
                                  neb_antenna_type_t type) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_rtk_base_antenna(cmd, sizeof(cmd), name, sn, setup_id, type);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_rtk_antenna_delta(neb_handle_t *handle, double height_m,
                                   double east_m, double north_m) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_rtk_antenna_delta(cmd, sizeof(cmd), height_m, east_m, north_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
