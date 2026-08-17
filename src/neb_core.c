#include "neb_core.h"
#include "neb_protocol.h"
#include "neb_serial_transport.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Upper bounds for internal scratch buffers. Commands are short; a single
// response line comfortably fits in NEB_LINE_MAX. Overlong lines (e.g. stray
// binary noise on the wire) are truncated rather than overflowing.
#define NEB_CMD_MAX 256
#define NEB_LINE_MAX 512

// Quiet window used when draining trailing query-payload lines after an OK
// acknowledgement. If nothing arrives within this window we assume the reply
// is complete.
#define NEB_PAYLOAD_QUIET_MS 200

// Hard ceiling on the total time spent draining query payload. The quiet window
// above ends the drain on an idle port, but a device with periodic output
// enabled (e.g. "GPGGA 1") never goes quiet -- without this bound the drain
// loop would read stream data forever and never return. Once this elapses we
// return whatever payload we captured.
#define NEB_PAYLOAD_MAX_MS 1000

const char *neb_strerror(neb_status_t status) {
  switch (status) {
  case NEB_OK:
    return "OK";
  case NEB_ERR_IO:
    return "serial I/O error";
  case NEB_ERR_TIMEOUT:
    return "timed out waiting for response";
  case NEB_ERR_NAK:
    return "command rejected by receiver";
  case NEB_ERR_PARSE:
    return "could not parse receiver response";
  case NEB_ERR_UNSUPPORTED:
    return "command not supported by this model";
  case NEB_ERR_INVALID_PARAM:
    return "invalid parameter";
  case NEB_ERR_INVALID_HANDLE:
    return "invalid or unopened handle";
  case NEB_ERR_OVERFLOW:
    return "command exceeded internal buffer";
  default:
    return "unknown error";
  }
}

neb_caps_t neb_caps_for_model(neb_model_t model) {
  // Derived from the manual's per-command "Applicable to:" lines. See the
  // NEB_CAP_* definitions in neb_core.h for the citations.
  switch (model) {
  case NEB_MODEL_UM960:
    return NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_ANTIJAM |
           NEB_CAP_ROVER_PROFILE | NEB_CAP_HEADING2_MODE |
           NEB_CAP_CONFIG_AGNSS | NEB_CAP_IONMODE | NEB_CAP_EVENT |
           NEB_CAP_PPS_ENABLE23 | NEB_CAP_STANDALONE | NEB_CAP_SBAS |
           NEB_CAP_MASK | NEB_CAP_ALGRESET | NEB_CAP_ADMIN | NEB_CAP_LOGGING;
  case NEB_MODEL_UM960L:
    return NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_MASK | NEB_CAP_ADMIN |
           NEB_CAP_LOGGING;
  case NEB_MODEL_UM980:
    return NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_PPP | NEB_CAP_AGNSS |
           NEB_CAP_ANTIJAM | NEB_CAP_ROVER_PROFILE | NEB_CAP_HEADING2_MODE |
           NEB_CAP_CONFIG_AGNSS | NEB_CAP_RTCM_B1C_B2A | NEB_CAP_IONMODE |
           NEB_CAP_MMP | NEB_CAP_EVENT | NEB_CAP_RTCMPHASERATE |
           NEB_CAP_PSRVELDRPOS | NEB_CAP_PPS_ENABLE23 | NEB_CAP_STANDALONE |
           NEB_CAP_SBAS | NEB_CAP_MASK | NEB_CAP_HEADING_OFFSET |
           NEB_CAP_SIGNALGROUP | NEB_CAP_ALGRESET | NEB_CAP_ADMIN |
           NEB_CAP_LOGGING;
  case NEB_MODEL_UM982:
    return NEB_CAP_MODE | NEB_CAP_CONFIG | NEB_CAP_PPP | NEB_CAP_AGNSS |
           NEB_CAP_HEADING | NEB_CAP_ROVER_PROFILE | NEB_CAP_HEADING2_MODE |
           NEB_CAP_CONFIG_AGNSS | NEB_CAP_RTCM_B1C_B2A | NEB_CAP_IONMODE |
           NEB_CAP_EVENT | NEB_CAP_RTCMPHASERATE | NEB_CAP_PPS_ENABLE23 |
           NEB_CAP_STANDALONE | NEB_CAP_SBAS | NEB_CAP_SBAS_TIMEOUT |
           NEB_CAP_MASK | NEB_CAP_MASK_CN0 | NEB_CAP_HEADING_OFFSET |
           NEB_CAP_SIGNALGROUP | NEB_CAP_ALGRESET | NEB_CAP_ADMIN |
           NEB_CAP_LOGGING;
  default:
    return 0;
  }
}

neb_status_t neb_open(neb_handle_t *handle, neb_model_t model,
                      const char *device, int baudrate) {
  if (!handle || !device)
    return NEB_ERR_INVALID_PARAM;

  handle->serial.fd = -1;
  handle->model = model;
  handle->caps = neb_caps_for_model(model);
  handle->timeout_ms = NEB_DEFAULT_TIMEOUT_MS;
  handle->is_open = 0;

  if (serial_open(&handle->serial, device, baudrate) < 0)
    return NEB_ERR_IO;

  // Back the transport with the handle's own serial_t. The command layer only
  // ever talks through handle->transport from here on.
  handle->transport = neb_serial_transport(&handle->serial);

  // Clear any bytes the device was already streaming so the first command's
  // acknowledgement is easy to find.
  serial_flush_input(&handle->serial);

  handle->is_open = 1;
  return NEB_OK;
}

void neb_close(neb_handle_t *handle) {
  if (!handle)
    return;
  serial_close(&handle->serial);
  handle->is_open = 0;
}

int neb_has_cap(const neb_handle_t *handle, neb_caps_t cap) {
  return handle && (handle->caps & cap) == cap;
}

static long now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

// Append `len` bytes of `src` to a caller buffer, NUL-terminating and never
// overrunning. `*used` tracks bytes written so far (excluding the terminator).
static void resp_append(char *dst, size_t cap, size_t *used, const char *src,
                        size_t len) {
  if (!dst || cap == 0)
    return;
  for (size_t i = 0; i < len && *used + 1 < cap; i++) {
    if (src[i] == '\0')
      continue; // keep the captured text NUL-free
    dst[(*used)++] = src[i];
  }
  dst[*used] = '\0';
}

neb_status_t neb_send_command(neb_handle_t *handle, const char *command,
                              char *response, size_t response_size) {
  if (!handle || !handle->is_open || !handle->transport.write ||
      !handle->transport.read)
    return NEB_ERR_INVALID_HANDLE;
  if (!command)
    return NEB_ERR_INVALID_PARAM;

  // Frame the command: abbreviated ASCII has no CRC, just a CR+LF terminator.
  char framed[NEB_CMD_MAX];
  int fn =
      snprintf(framed, sizeof(framed), "%s%s", command, NEB_LINE_TERMINATOR);
  if (fn < 0)
    return NEB_ERR_PARSE;
  if ((size_t)fn >= sizeof(framed))
    return NEB_ERR_OVERFLOW;

  if (handle->transport.write(handle->transport.ctx, (const uint8_t *)framed,
                              (size_t)fn) < 0)
    return NEB_ERR_IO;

  size_t resp_used = 0;
  if (response && response_size)
    response[0] = '\0';

  // Line-oriented scan for the "$command,...,response: ..." acknowledgement.
  // We discard non-matching lines so that streaming data or leading garbage
  // (observed as stray bytes before the reply) does not derail parsing.
  char line[NEB_LINE_MAX];
  size_t line_len = 0;
  long deadline = now_ms() + handle->timeout_ms;
  neb_status_t result = NEB_ERR_TIMEOUT;
  int acknowledged = 0;

  while (!acknowledged) {
    long remaining = deadline - now_ms();
    if (remaining <= 0)
      break;

    uint8_t chunk[256];
    int r = handle->transport.read(handle->transport.ctx, chunk, sizeof(chunk),
                                   (int)remaining);
    if (r < 0)
      return NEB_ERR_IO;
    if (r == 0)
      break; // timed out

    for (int i = 0; i < r && !acknowledged; i++) {
      uint8_t b = chunk[i];
      if (b == '\n') {
        line[line_len] = '\0';

        char *match = strstr(line, NEB_RESP_PREFIX);
        if (match) {
          // Classify: an explicit "response: OK" is the only success form;
          // any other acknowledgement (PARSING FAILED, ...) is a rejection.
          if (strstr(match, NEB_RESP_OK))
            result = NEB_OK;
          else
            result = NEB_ERR_NAK;

          resp_append(response, response_size, &resp_used, match,
                      strlen(match));
          resp_append(response, response_size, &resp_used, "\n", 1);
          acknowledged = 1;

          // Query payload (e.g. the "#MODE,..." record) can arrive in the same
          // read chunk right after the ack. Capture the remainder of this chunk
          // now so it is not lost; the drain loop below handles any payload
          // that lands in later chunks.
          if (result == NEB_OK && response && response_size && i + 1 < r)
            resp_append(response, response_size, &resp_used,
                        (const char *)(chunk + i + 1), (size_t)(r - (i + 1)));
        }
        line_len = 0;
      } else if (b != '\r' && b != '\0') {
        if (line_len < sizeof(line) - 1)
          line[line_len++] = (char)b;
        // Overlong line: drop the excess rather than overflow.
      }
    }
  }

  if (!acknowledged)
    return result; // NEB_ERR_TIMEOUT

  // For query commands the caller passes a response buffer; drain the trailing
  // payload line(s) (e.g. the "#MODE,..." record) until the port goes quiet or
  // the overall budget elapses. The budget guards against a device streaming
  // periodic output, where the port never goes quiet and the loop would
  // otherwise read forever.
  if (result == NEB_OK && response && response_size) {
    long drain_deadline = now_ms() + NEB_PAYLOAD_MAX_MS;
    while (resp_used + 1 < response_size) {
      long remaining = drain_deadline - now_ms();
      if (remaining <= 0)
        break;
      int quiet = remaining < NEB_PAYLOAD_QUIET_MS ? (int)remaining
                                                   : NEB_PAYLOAD_QUIET_MS;
      uint8_t chunk[256];
      int r = handle->transport.read(handle->transport.ctx, chunk,
                                     sizeof(chunk), quiet);
      if (r <= 0)
        break;
      resp_append(response, response_size, &resp_used, (const char *)chunk,
                  (size_t)r);
    }
  }

  return result;
}
