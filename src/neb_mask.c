#include "neb_mask.h"
#include "neb_protocol.h"

#include <stdio.h>

// ===========================================================================
// Builders -- pure, no I/O.
// ===========================================================================

// Manual §5.1 -- MASK (query)
neb_status_t neb_build_mask_query(char *out, size_t len) {
  // "MASK"
  int n = snprintf(out, len, "%s", NEB_CMD_MASK);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2 -- MASK <system>
neb_status_t neb_build_mask_gnss(char *out, size_t len, neb_gnss_t gnss) {
  const char *gnss_str = neb_gnss_str(gnss);
  if (!gnss_str)
    return NEB_ERR_INVALID_PARAM;

  // "MASK GPS"
  int n = snprintf(out, len, "%s %s", NEB_CMD_MASK, gnss_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.3 -- UNMASK <system>
neb_status_t neb_build_unmask_gnss(char *out, size_t len, neb_gnss_t gnss) {
  const char *gnss_str = neb_gnss_str(gnss);
  if (!gnss_str)
    return NEB_ERR_INVALID_PARAM;

  // "UNMASK GPS"
  int n = snprintf(out, len, "%s %s", NEB_CMD_UNMASK, gnss_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-2 -- MASK <elevation>
neb_status_t neb_build_mask_elevation(char *out, size_t len, int angle_deg) {
  // Documented range: -90..90 degrees.
  if (angle_deg < -90 || angle_deg > 90)
    return NEB_ERR_INVALID_PARAM;

  // "MASK 10"
  int n = snprintf(out, len, "%s %d", NEB_CMD_MASK, angle_deg);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-2 -- MASK <elevation> <system>
neb_status_t neb_build_mask_elevation_gnss(char *out, size_t len, int angle_deg,
                                           neb_gnss_t gnss) {
  const char *gnss_str = neb_gnss_str(gnss);
  if (!gnss_str)
    return NEB_ERR_INVALID_PARAM;
  if (angle_deg < -90 || angle_deg > 90)
    return NEB_ERR_INVALID_PARAM;

  // "MASK 10 GPS"
  int n = snprintf(out, len, "%s %d %s", NEB_CMD_MASK, angle_deg, gnss_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-3 -- MASK <system> PRN <id>
neb_status_t neb_build_mask_satellite(char *out, size_t len, neb_gnss_t gnss,
                                      unsigned prn) {
  const char *gnss_str = neb_gnss_str(gnss);
  if (!gnss_str)
    return NEB_ERR_INVALID_PARAM;
  // PRN must be positive; exact per-system ranges (Table 7-52) are the device's
  // to enforce.
  if (prn < 1)
    return NEB_ERR_INVALID_PARAM;

  // "MASK GPS PRN 10"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_MASK, gnss_str, NEB_TOK_PRN,
                   prn);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.3, Table 5-7 -- UNMASK <system> PRN <id>
neb_status_t neb_build_unmask_satellite(char *out, size_t len, neb_gnss_t gnss,
                                        unsigned prn) {
  const char *gnss_str = neb_gnss_str(gnss);
  if (!gnss_str)
    return NEB_ERR_INVALID_PARAM;
  if (prn < 1)
    return NEB_ERR_INVALID_PARAM;

  // "UNMASK GPS PRN 10"
  int n = snprintf(out, len, "%s %s %s %u", NEB_CMD_UNMASK, gnss_str,
                   NEB_TOK_PRN, prn);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2 -- MASK <frequency>
neb_status_t neb_build_mask_frequency(char *out, size_t len,
                                      neb_gnss_freq_t freq) {
  const char *freq_str = neb_gnss_freq_str(freq);
  if (!freq_str)
    return NEB_ERR_INVALID_PARAM;

  // "MASK B1"
  int n = snprintf(out, len, "%s %s", NEB_CMD_MASK, freq_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.3 -- UNMASK <frequency>
neb_status_t neb_build_unmask_frequency(char *out, size_t len,
                                        neb_gnss_freq_t freq) {
  const char *freq_str = neb_gnss_freq_str(freq);
  if (!freq_str)
    return NEB_ERR_INVALID_PARAM;

  // "UNMASK B1"
  int n = snprintf(out, len, "%s %s", NEB_CMD_UNMASK, freq_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-2 -- MASK <elevation> <frequency>
neb_status_t neb_build_mask_elevation_frequency(char *out, size_t len,
                                                int angle_deg,
                                                neb_gnss_freq_t freq) {
  const char *freq_str = neb_gnss_freq_str(freq);
  if (!freq_str)
    return NEB_ERR_INVALID_PARAM;
  if (angle_deg < -90 || angle_deg > 90)
    return NEB_ERR_INVALID_PARAM;

  // "MASK 10 B1"
  int n = snprintf(out, len, "%s %d %s", NEB_CMD_MASK, angle_deg, freq_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-5 -- MASK RTCMCN0 <cn0>
neb_status_t neb_build_mask_rtcmcn0(char *out, size_t len, unsigned cn0) {
  // "MASK RTCMCN0 35"
  int n = snprintf(out, len, "%s %s %u", NEB_CMD_MASK, NEB_TOK_RTCMCN0, cn0);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-5 -- MASK RTCMCN0 <cn0> <frequency>
neb_status_t neb_build_mask_rtcmcn0_frequency(char *out, size_t len,
                                              unsigned cn0,
                                              neb_gnss_freq_t freq) {
  const char *freq_str = neb_gnss_freq_str(freq);
  if (!freq_str)
    return NEB_ERR_INVALID_PARAM;

  // "MASK RTCMCN0 35 L1"
  int n = snprintf(out, len, "%s %s %u %s", NEB_CMD_MASK, NEB_TOK_RTCMCN0, cn0,
                   freq_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-5 -- MASK CN0 <cn0>
neb_status_t neb_build_mask_cn0(char *out, size_t len, unsigned cn0) {
  // "MASK CN0 40"
  int n = snprintf(out, len, "%s %s %u", NEB_CMD_MASK, NEB_TOK_CN0, cn0);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// Manual §5.2, Table 5-5 -- MASK CN0 <cn0> <frequency>
neb_status_t neb_build_mask_cn0_frequency(char *out, size_t len, unsigned cn0,
                                          neb_gnss_freq_t freq) {
  const char *freq_str = neb_gnss_freq_str(freq);
  if (!freq_str)
    return NEB_ERR_INVALID_PARAM;

  // "MASK CN0 40 B1I"
  int n = snprintf(out, len, "%s %s %u %s", NEB_CMD_MASK, NEB_TOK_CN0, cn0,
                   freq_str);
  if (n < 0 || (size_t)n >= len)
    return NEB_ERR_OVERFLOW;
  return NEB_OK;
}

// ===========================================================================
// Wrappers -- capability check, build, send. MASK/UNMASK are on every model.
// ===========================================================================

neb_status_t neb_mask_query(neb_handle_t *handle, char *response,
                            size_t response_size) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_query(cmd, sizeof(cmd));
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, response, response_size);
}

neb_status_t neb_mask_gnss(neb_handle_t *handle, neb_gnss_t gnss) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_gnss(cmd, sizeof(cmd), gnss);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_unmask_gnss(neb_handle_t *handle, neb_gnss_t gnss) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_unmask_gnss(cmd, sizeof(cmd), gnss);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_elevation(neb_handle_t *handle, int angle_deg) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_elevation(cmd, sizeof(cmd), angle_deg);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_elevation_gnss(neb_handle_t *handle, int angle_deg,
                                     neb_gnss_t gnss) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_mask_elevation_gnss(cmd, sizeof(cmd), angle_deg, gnss);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_satellite(neb_handle_t *handle, neb_gnss_t gnss,
                                unsigned prn) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_satellite(cmd, sizeof(cmd), gnss, prn);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_unmask_satellite(neb_handle_t *handle, neb_gnss_t gnss,
                                  unsigned prn) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_unmask_satellite(cmd, sizeof(cmd), gnss, prn);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_frequency(neb_handle_t *handle, neb_gnss_freq_t freq) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_frequency(cmd, sizeof(cmd), freq);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_unmask_frequency(neb_handle_t *handle, neb_gnss_freq_t freq) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_unmask_frequency(cmd, sizeof(cmd), freq);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_elevation_frequency(neb_handle_t *handle, int angle_deg,
                                          neb_gnss_freq_t freq) {
  if (!neb_has_cap(handle, NEB_CAP_MASK))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_mask_elevation_frequency(cmd, sizeof(cmd), angle_deg, freq);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_rtcmcn0(neb_handle_t *handle, unsigned cn0) {
  if (!neb_has_cap(handle, NEB_CAP_MASK_CN0))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_rtcmcn0(cmd, sizeof(cmd), cn0);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_rtcmcn0_frequency(neb_handle_t *handle, unsigned cn0,
                                        neb_gnss_freq_t freq) {
  if (!neb_has_cap(handle, NEB_CAP_MASK_CN0))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st =
      neb_build_mask_rtcmcn0_frequency(cmd, sizeof(cmd), cn0, freq);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_cn0(neb_handle_t *handle, unsigned cn0) {
  if (!neb_has_cap(handle, NEB_CAP_MASK_CN0))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_cn0(cmd, sizeof(cmd), cn0);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}

neb_status_t neb_mask_cn0_frequency(neb_handle_t *handle, unsigned cn0,
                                    neb_gnss_freq_t freq) {
  if (!neb_has_cap(handle, NEB_CAP_MASK_CN0))
    return NEB_ERR_UNSUPPORTED;

  char cmd[NEB_CMD_BUF_LEN];
  neb_status_t st = neb_build_mask_cn0_frequency(cmd, sizeof(cmd), cn0, freq);
  if (st != NEB_OK)
    return st;

  return neb_send_command(handle, cmd, NULL, 0);
}
