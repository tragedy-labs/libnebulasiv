#include "neb_config.h"
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O. These are the manual-as-spec surface: each formats
// the exact wire string documented in the manual.
// ===========================================================================

// Manual §4.1 -- Query the Receiver's Configuration
neb_status_t neb_build_config_query(char *out, size_t len) {
  // "CONFIG"
  int n = snprintf(out, len, "%s", NEB_CMD_CONFIG);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.19 -- CONFIG PPP Enable
neb_status_t neb_build_config_ppp_enable(char *out, size_t len,
                                         neb_ppp_mode_t mode) {
  const char *mode_str = neb_ppp_mode_str(mode);
  if (!mode_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG PPP Enable <mode>"
  int n = snprintf(out, len, "%s %s %s %s", NEB_CMD_CONFIG, NEB_TOK_PPP,
                   NEB_TOK_ENABLE, mode_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.19 -- CONFIG PPP Disable
neb_status_t neb_build_config_ppp_disable(char *out, size_t len) {
  // "CONFIG PPP Disable"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_PPP,
                   NEB_TOK_DISABLE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// The eight documented serial baud rates (Manual §4.2, Table 4-4).
static int is_supported_baud(unsigned baud) {
  switch (baud) {
  case 9600:
  case 19200:
  case 38400:
  case 57600:
  case 115200:
  case 230400:
  case 460800:
  case 921600:
    return 1;
  default:
    return 0;
  }
}

// Manual §4.2 -- CONFIG COMx <baud>
neb_status_t neb_build_config_serial(char *out, size_t len, neb_com_port_t port,
                                     unsigned baud) {
  const char *port_str = neb_com_port_str(port);
  if (!port_str)
    return NEB_ERR_INVALID_PARAM;
  if (!is_supported_baud(baud))
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG COM1 115200"
  int n = snprintf(out, len, "%s %s %u", NEB_CMD_CONFIG, port_str, baud);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.14 -- CONFIG NMEA0183 <version>
neb_status_t neb_build_config_nmea_version(char *out, size_t len,
                                           neb_nmea_version_t version) {
  const char *version_str = neb_nmea_version_str(version);
  if (!version_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG NMEA0183 V410"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_NMEA0183,
                   version_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.20 -- CONFIG ANTIJAM <mode>
neb_status_t neb_build_config_antijam(char *out, size_t len,
                                      neb_antijam_mode_t mode) {
  const char *mode_str = neb_antijam_mode_str(mode);
  if (!mode_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG ANTIJAM AUTO"
  int n =
      snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_ANTIJAM, mode_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.18 -- CONFIG AGNSS Enable
neb_status_t neb_build_config_agnss_enable(char *out, size_t len) {
  // "CONFIG AGNSS Enable"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_AGNSS,
                   NEB_TOK_ENABLE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.18 -- CONFIG AGNSS Disable
neb_status_t neb_build_config_agnss_disable(char *out, size_t len) {
  // "CONFIG AGNSS Disable"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_AGNSS,
                   NEB_TOK_DISABLE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.4 -- CONFIG UNDULATION Auto
neb_status_t neb_build_config_undulation_auto(char *out, size_t len) {
  // "CONFIG UNDULATION Auto"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_UNDULATION,
                   NEB_TOK_AUTO);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.4, Table 4-7 -- CONFIG UNDULATION <separation>
neb_status_t neb_build_config_undulation(char *out, size_t len,
                                         double separation_m) {
  // Documented range: -1000.0000..+1000.0000 m, four decimals.
  if (separation_m < -1000.0 || separation_m > 1000.0)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG UNDULATION 9.7000"
  int n = snprintf(out, len, "%s %s %.4f", NEB_CMD_CONFIG, NEB_TOK_UNDULATION,
                   separation_m);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.12, Table 4-17 -- CONFIG SMOOTH RTKHEIGHT <epochs>
neb_status_t neb_build_config_smooth_rtkheight(char *out, size_t len,
                                               unsigned epochs) {
  // Documented range: 0..100 epochs.
  if (epochs > 100)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SMOOTH RTKHEIGHT 10"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_SMOOTH,
                   NEB_TOK_RTKHEIGHT, epochs);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.12, Table 4-17 -- CONFIG SMOOTH HEADING <epochs>
neb_status_t neb_build_config_smooth_heading(char *out, size_t len,
                                             unsigned epochs) {
  // Documented range: 0..100 epochs.
  if (epochs > 100)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SMOOTH HEADING 10"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_SMOOTH,
                   NEB_TOK_HEADING, epochs);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.12 -- CONFIG SMOOTH PSRVEL enable (lowercase per the manual
// example)
neb_status_t neb_build_config_smooth_psrvel_enable(char *out, size_t len) {
  // "CONFIG SMOOTH PSRVEL enable"
  int n = snprintf(out, len, "%s %s %s %s", NEB_CMD_CONFIG, NEB_TOK_SMOOTH,
                   NEB_TOK_PSRVEL, NEB_TOK_ENABLE_LC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.12 -- CONFIG SMOOTH PSRVEL disable
neb_status_t neb_build_config_smooth_psrvel_disable(char *out, size_t len) {
  // "CONFIG SMOOTH PSRVEL disable"
  int n = snprintf(out, len, "%s %s %s %s", NEB_CMD_CONFIG, NEB_TOK_SMOOTH,
                   NEB_TOK_PSRVEL, NEB_TOK_DISABLE_LC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.15, Table 4-20 -- CONFIG RTCMB1CB2a Enable
neb_status_t neb_build_config_rtcm_b1c_b2a_enable(char *out, size_t len) {
  // "CONFIG RTCMB1CB2a Enable"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTCMB1CB2A,
                   NEB_TOK_ENABLE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.15, Table 4-20 -- CONFIG RTCMB1CB2a Disable
neb_status_t neb_build_config_rtcm_b1c_b2a_disable(char *out, size_t len) {
  // "CONFIG RTCMB1CB2a Disable"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTCMB1CB2A,
                   NEB_TOK_DISABLE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.25, Table 4-33 -- CONFIG IONMODE <mode>
neb_status_t neb_build_config_ionmode(char *out, size_t len,
                                      neb_ionmode_t mode) {
  const char *mode_str = neb_ionmode_str(mode);
  if (!mode_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG IONMODE GPSK8"
  int n =
      snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_IONMODE, mode_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.13, Table 4-18 -- CONFIG MMP ENABLE (uppercase per the manual)
neb_status_t neb_build_config_mmp_enable(char *out, size_t len) {
  // "CONFIG MMP ENABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_MMP,
                   NEB_TOK_ENABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.13, Table 4-18 -- CONFIG MMP DISABLE
neb_status_t neb_build_config_mmp_disable(char *out, size_t len) {
  // "CONFIG MMP DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_MMP,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.11, Table 4-16 -- CONFIG EVENT DISABLE
neb_status_t neb_build_config_event_disable(char *out, size_t len) {
  // "CONFIG EVENT DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_EVENT,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.11, Table 4-16 -- CONFIG EVENT ENABLE <edge> <tguard>. Casing
// follows the manual's Input Example ("CONFIG EVENT ENABLE POSITIVE 10"), which
// differs from the table's "Enable"; the field is case-insensitive on the wire.
neb_status_t neb_build_config_event_enable(char *out, size_t len,
                                           neb_polarity_t polarity,
                                           unsigned tguard_ms) {
  const char *polarity_str = neb_polarity_str(polarity);
  if (!polarity_str)
    return NEB_ERR_INVALID_PARAM;

  // Documented TGUARD range: 2..3599999 ms (Manual §4.11, Table 4-16).
  if (tguard_ms < 2 || tguard_ms > 3599999)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG EVENT ENABLE POSITIVE 10"
  int n = snprintf(out, len, "%s %s %s %s %u", NEB_CMD_CONFIG, NEB_TOK_EVENT,
                   NEB_TOK_ENABLE_UC, polarity_str, tguard_ms);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.16, Table 4-21 -- CONFIG RTCMPHASERATE POSITIVE
neb_status_t neb_build_config_rtcmphaserate_positive(char *out, size_t len) {
  // "CONFIG RTCMPHASERATE POSITIVE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTCMPHASERATE,
                   NEB_TOK_POSITIVE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.16, Table 4-21 -- CONFIG RTCMPHASERATE NEGATIVE
neb_status_t neb_build_config_rtcmphaserate_negative(char *out, size_t len) {
  // "CONFIG RTCMPHASERATE NEGATIVE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_RTCMPHASERATE,
                   NEB_TOK_NEGATIVE);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.17, Table 4-22 -- CONFIG PSRVELDRPOS ENABLE (uppercase per example)
neb_status_t neb_build_config_psrveldrpos_enable(char *out, size_t len) {
  // "CONFIG PSRVELDRPOS ENABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_PSRVELDRPOS,
                   NEB_TOK_ENABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.17, Table 4-22 -- CONFIG PSRVELDRPOS DISABLE
neb_status_t neb_build_config_psrveldrpos_disable(char *out, size_t len) {
  // "CONFIG PSRVELDRPOS DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_PSRVELDRPOS,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.3, Table 4-5 -- CONFIG PPS DISABLE
neb_status_t neb_build_config_pps_disable(char *out, size_t len) {
  // "CONFIG PPS DISABLE"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_PPS,
                   NEB_TOK_DISABLE_UC);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.3, Table 4-6 -- CONFIG PPS <mode> <timeref> <polarity> <width>
// <period> <rfdelay> <userdelay>
neb_status_t neb_build_config_pps_enable(char *out, size_t len,
                                         neb_pps_mode_t mode,
                                         neb_pps_timeref_t timeref,
                                         neb_polarity_t polarity,
                                         unsigned width_us, unsigned period_ms,
                                         int rf_delay_ns, int user_delay_ns) {
  const char *mode_str = neb_pps_mode_str(mode);
  const char *timeref_str = neb_pps_timeref_str(timeref);
  const char *polarity_str = neb_polarity_str(polarity);
  if (!mode_str || !timeref_str || !polarity_str)
    return NEB_ERR_INVALID_PARAM;

  // Documented bounds (Manual §4.3, Table 4-6).
  if (period_ms < 50 || period_ms > 20000)
    return NEB_ERR_INVALID_PARAM;
  // "Pulse width, smaller than the period." Width is microseconds, period is
  // milliseconds; compare in a common unit without overflow.
  if ((unsigned long)width_us >= (unsigned long)period_ms * 1000UL)
    return NEB_ERR_INVALID_PARAM;
  if (rf_delay_ns < -32768 || rf_delay_ns > 32767)
    return NEB_ERR_INVALID_PARAM;
  if (user_delay_ns < -32768 || user_delay_ns > 32767)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG PPS ENABLE GPS POSITIVE 500000 1000 0 0"
  int n = snprintf(out, len, "%s %s %s %s %s %u %u %d %d", NEB_CMD_CONFIG,
                   NEB_TOK_PPS, mode_str, timeref_str, polarity_str, width_us,
                   period_ms, rf_delay_ns, user_delay_ns);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.19, Table 4-25 -- CONFIG PPP CONVERGE
neb_status_t neb_build_config_ppp_converge(char *out, size_t len,
                                           double hor_std_cm,
                                           double ver_std_cm) {
  // The manual documents these as standard-deviation thresholds in centimetres
  // but gives no numeric range. Reject values meaningless for a std-dev
  // threshold (<= 0), and -- as a formatting/finiteness guard, not a device
  // rule -- anything >= 1e6 cm: such values (and NaN/Inf) are far beyond any
  // real threshold and would render in scientific notation via %g below,
  // producing an unparseable command. The device validates the exact upper end.
  if (!(hor_std_cm > 0.0 && hor_std_cm < 1e6) ||
      !(ver_std_cm > 0.0 && ver_std_cm < 1e6))
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG PPP CONVERGE <hor> <ver>" -- %g reproduces the manual's compact
  // examples ("10 20", "2.5 3.5"); the bound above keeps it out of the
  // exponent range.
  int n = snprintf(out, len, "%s %s %s %g %g", NEB_CMD_CONFIG, NEB_TOK_PPP,
                   NEB_TOK_CONVERGE, hor_std_cm, ver_std_cm);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.21, Table 4-28 -- CONFIG SIGNALGROUP <master>
neb_status_t neb_build_config_signalgroup(char *out, size_t len,
                                          unsigned master) {
  // TypeNum range 0..7 (Table 4-28).
  if (master > 7)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SIGNALGROUP 1"
  int n = snprintf(out, len, "%s %s %u", NEB_CMD_CONFIG, NEB_TOK_SIGNALGROUP,
                   master);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.21, Table 4-27 -- CONFIG SIGNALGROUP <master> <slave>
neb_status_t neb_build_config_signalgroup_dual(char *out, size_t len,
                                               unsigned master,
                                               unsigned slave) {
  if (master > 7 || slave > 7)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG SIGNALGROUP 2 3"
  int n = snprintf(out, len, "%s %s %u %u", NEB_CMD_CONFIG, NEB_TOK_SIGNALGROUP,
                   master, slave);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §4.24, Table 4-32 -- CONFIG ALGRESET <type>
neb_status_t neb_build_config_algreset(char *out, size_t len,
                                       neb_algreset_type_t type) {
  const char *type_str = neb_algreset_type_str(type);
  if (!type_str)
    return NEB_ERR_INVALID_PARAM;

  // "CONFIG ALGRESET RTK1"
  int n = snprintf(out, len, "%s %s %s", NEB_CMD_CONFIG, NEB_TOK_ALGRESET,
                   type_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send.
// ===========================================================================

neb_status_t neb_config_query(neb_handle_t *handle, char *response,
                              size_t response_size) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_query(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, response, response_size);
}

neb_status_t neb_config_ppp_enable(neb_handle_t *handle, neb_ppp_mode_t mode) {
  if (!neb_has_cap(handle, NEB_CAP_PPP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_ppp_enable(cmd, sizeof(cmd), mode);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_ppp_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_PPP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_ppp_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_ppp_converge(neb_handle_t *handle, double hor_std_cm,
                                     double ver_std_cm) {
  if (!neb_has_cap(handle, NEB_CAP_PPP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_config_ppp_converge(cmd, sizeof(cmd), hor_std_cm, ver_std_cm);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_serial(neb_handle_t *handle, neb_com_port_t port,
                               unsigned baud) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_serial(cmd, sizeof(cmd), port, baud);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_nmea_version(neb_handle_t *handle,
                                     neb_nmea_version_t version) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_nmea_version(cmd, sizeof(cmd), version);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_antijam(neb_handle_t *handle, neb_antijam_mode_t mode) {
  if (!neb_has_cap(handle, NEB_CAP_ANTIJAM))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_antijam(cmd, sizeof(cmd), mode);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_agnss_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG_AGNSS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_agnss_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_agnss_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG_AGNSS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_agnss_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_undulation_auto(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_undulation_auto(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_undulation(neb_handle_t *handle, double separation_m) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_undulation(cmd, sizeof(cmd), separation_m);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_smooth_rtkheight(neb_handle_t *handle,
                                         unsigned epochs) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_smooth_rtkheight(cmd, sizeof(cmd), epochs);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_smooth_heading(neb_handle_t *handle, unsigned epochs) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_smooth_heading(cmd, sizeof(cmd), epochs);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_smooth_psrvel_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_smooth_psrvel_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_smooth_psrvel_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_smooth_psrvel_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_rtcm_b1c_b2a_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_RTCM_B1C_B2A))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_rtcm_b1c_b2a_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_rtcm_b1c_b2a_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_RTCM_B1C_B2A))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_rtcm_b1c_b2a_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_ionmode(neb_handle_t *handle, neb_ionmode_t mode) {
  if (!neb_has_cap(handle, NEB_CAP_IONMODE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_ionmode(cmd, sizeof(cmd), mode);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_mmp_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_MMP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_mmp_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_mmp_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_MMP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_mmp_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_event_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_EVENT))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_event_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_event_enable(neb_handle_t *handle,
                                     neb_polarity_t polarity,
                                     unsigned tguard_ms) {
  if (!neb_has_cap(handle, NEB_CAP_EVENT))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_config_event_enable(cmd, sizeof(cmd), polarity, tguard_ms);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_rtcmphaserate_positive(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_RTCMPHASERATE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_rtcmphaserate_positive(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_rtcmphaserate_negative(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_RTCMPHASERATE))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_rtcmphaserate_negative(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_psrveldrpos_enable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_PSRVELDRPOS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_psrveldrpos_enable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_psrveldrpos_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_PSRVELDRPOS))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_psrveldrpos_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_pps_disable(neb_handle_t *handle) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_pps_disable(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_pps_enable(neb_handle_t *handle, neb_pps_mode_t mode,
                                   neb_pps_timeref_t timeref,
                                   neb_polarity_t polarity, unsigned width_us,
                                   unsigned period_ms, int rf_delay_ns,
                                   int user_delay_ns) {
  if (!neb_has_cap(handle, NEB_CAP_CONFIG))
    return NEB_ERR_UNSUPPORTED;
  // ENABLE2/ENABLE3 are unavailable on the UM960L (Manual §4.3 fn2).
  if ((mode == NEB_PPS_ENABLE2 || mode == NEB_PPS_ENABLE3) &&
      !neb_has_cap(handle, NEB_CAP_PPS_ENABLE23))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_pps_enable(cmd, sizeof(cmd), mode, timeref,
                                                polarity, width_us, period_ms,
                                                rf_delay_ns, user_delay_ns);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_signalgroup(neb_handle_t *handle, unsigned master) {
  if (!neb_has_cap(handle, NEB_CAP_SIGNALGROUP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_signalgroup(cmd, sizeof(cmd), master);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_signalgroup_dual(neb_handle_t *handle, unsigned master,
                                         unsigned slave) {
  if (!neb_has_cap(handle, NEB_CAP_SIGNALGROUP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_config_signalgroup_dual(cmd, sizeof(cmd), master, slave);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_config_algreset(neb_handle_t *handle,
                                 neb_algreset_type_t type) {
  if (!neb_has_cap(handle, NEB_CAP_ALGRESET))
    return NEB_ERR_UNSUPPORTED;
  // Per-value model restrictions (Manual §4.24, Table 4-32): the slave-antenna
  // and heading resets need a UM982; the PPP reset needs a PPP-capable model.
  if ((type == NEB_ALGRESET_RTK2 || type == NEB_ALGRESET_HEADING) &&
      handle->model != NEB_MODEL_UM982)
    return NEB_ERR_UNSUPPORTED;
  if (type == NEB_ALGRESET_PPP && !neb_has_cap(handle, NEB_CAP_PPP))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_config_algreset(cmd, sizeof(cmd), type);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
