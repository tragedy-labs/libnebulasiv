// neb_core.h
//
// Core context/handle for a Unicore NebulasIV (N4) receiver: the handle
// struct, the status enum returned by every command, the chip-model and
// capability model, and neb_send_command() -- the single choke point every
// capability module funnels through.
#ifndef NEB_CORE_H
#define NEB_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "serial.h"

// ---------------------------------------------------------------------------
// Status codes -- returned by every command function.
// ---------------------------------------------------------------------------
typedef enum {
  NEB_OK = 0,             // command accepted (device replied "response: OK")
  NEB_ERR_IO,             // serial write/read failure
  NEB_ERR_TIMEOUT,        // no acknowledgement within the timeout window
  NEB_ERR_NAK,            // device rejected the command (PARSING FAILED, ...)
  NEB_ERR_PARSE,          // a reply arrived but could not be interpreted
  NEB_ERR_UNSUPPORTED,    // command not supported by this chip model
  NEB_ERR_INVALID_PARAM,  // a parameter failed validation before being sent
  NEB_ERR_INVALID_HANDLE, // NULL handle or handle not open
  NEB_ERR_OVERFLOW        // formatted command exceeded the internal buffer
} neb_status_t;

// Human-readable description of a status code, for logging/debugging.
const char *neb_strerror(neb_status_t status);

// Size of the stack buffer a command wrapper uses to hold a formatted command
// before sending. Builders take an explicit `len`, so this is just the
// conventional size wrappers allocate; it must be large enough for the longest
// command the library produces. Builders that would overflow it return
// NEB_ERR_OVERFLOW rather than truncating.
#define NEB_CMD_BUF_LEN 128

// ---------------------------------------------------------------------------
// Transport interface.
// ---------------------------------------------------------------------------
// neb_core talks to the byte layer through this small interface rather than
// calling termios directly, so tests can substitute a fake. The termios serial
// layer is the real implementation (see neb_serial_transport.h).
//
//   write: send `len` bytes; returns bytes written (>=0) or <0 on error.
//   read:  read up to `len` bytes, waiting at most `timeout_ms` for the first;
//          returns bytes read (>0), 0 on timeout with no data, or <0 on error.
typedef struct {
  int (*write)(void *ctx, const uint8_t *data, size_t len);
  int (*read)(void *ctx, uint8_t *buf, size_t len, int timeout_ms);
  void *ctx;
} neb_transport_t;

// ---------------------------------------------------------------------------
// Chip model.
// ---------------------------------------------------------------------------
// The N4 command manual covers these four models. Capability differences are
// documented per-command via the manual's "Applicable to:" lines, which drive
// the capability bitfield below.
typedef enum {
  NEB_MODEL_UM960,
  NEB_MODEL_UM960L,
  NEB_MODEL_UM980,
  NEB_MODEL_UM982
} neb_model_t;

// ---------------------------------------------------------------------------
// Capability bitfield.
// ---------------------------------------------------------------------------
// C cannot stop a caller from including a header for a command their chip does
// not support, so this bitfield is the actual enforcement layer: each command
// in a capability module checks the relevant flag at entry and returns
// NEB_ERR_UNSUPPORTED rather than sending something the hardware would NAK (or,
// worse, silently ignore -- see the unknown-command timeout behavior).
//
// Each flag's model set is cited to the manual's "Applicable to:" line.
//
// Capability granularity is model-level: a flag answers "can this chip model
// do X?". Some commands additionally gate individual parameters on the running
// firmware *build* version (e.g. certain rover profiles need UM980 Build7923+),
// which the model alone cannot reveal. For those, the cap check confirms only
// that the model is in the right family; the running build is enforced by the
// device itself, which returns NEB_ERR_NAK if it lacks support. Builders with
// this property carry a "Build-gated:" note in their doc comment.
typedef uint32_t neb_caps_t;

#define NEB_CAP_MODE (1u << 0)   // MODE command      (Manual §3, all models)
#define NEB_CAP_CONFIG (1u << 1) // CONFIG command    (Manual §4, all models)
#define NEB_CAP_PPP (1u << 2) // CONFIG PPP        (Manual §4.19: UM980, UM982)
#define NEB_CAP_HEADING (1u << 3) // CONFIG HEADING    (Manual §4.8: UM982 only)
#define NEB_CAP_AGNSS                                                          \
  (1u << 4) // AGNSS             (Manual §2/§6: UM980, UM982)
#define NEB_CAP_ANTIJAM                                                        \
  (1u << 5) // CONFIG ANTIJAM    (Manual §4.20: UM960, UM980)
#define NEB_CAP_ROVER_PROFILE                                                  \
  (1u << 6) // MODE ROVER <profile> (Manual §3.6: UM960, UM980, UM982)
#define NEB_CAP_HEADING2_MODE                                                  \
  (1u << 7) // MODE HEADING2        (Manual §3.7: UM960, UM980, UM982)
#define NEB_CAP_CONFIG_AGNSS                                                   \
  (1u << 8) // CONFIG AGNSS         (Manual §4.18: UM960, UM980, UM982)
#define NEB_CAP_RTCM_B1C_B2A                                                   \
  (1u << 9) // CONFIG RTCMB1CB2a    (Manual §4.15: UM980, UM982)
#define NEB_CAP_IONMODE                                                        \
  (1u << 10) // CONFIG IONMODE      (Manual §4.25: UM960, UM980, UM982)
#define NEB_CAP_MMP (1u << 11) // CONFIG MMP          (Manual §4.13: UM980 only)
#define NEB_CAP_EVENT                                                          \
  (1u << 12) // CONFIG EVENT        (Manual §4.11: UM960, UM980, UM982)
#define NEB_CAP_RTCMPHASERATE                                                  \
  (1u << 13) // CONFIG RTCMPHASERATE (Manual §4.16: UM980, UM982)
#define NEB_CAP_PSRVELDRPOS                                                    \
  (1u << 14) // CONFIG PSRVELDRPOS   (Manual §4.17: UM980 only)
#define NEB_CAP_PPS_ENABLE23                                                   \
  (1u << 15) // CONFIG PPS ENABLE2/3 (Manual §4.3 fn2: UM960, UM980, UM982;
             // the base PPS command is all models via NEB_CAP_CONFIG)
#define NEB_CAP_STANDALONE                                                     \
  (1u << 16) // CONFIG STANDALONE   (Manual §4.7: UM960, UM980, UM982)
#define NEB_CAP_SBAS                                                           \
  (1u << 17) // CONFIG SBAS EN/DIS  (Manual §4.10: UM960, UM980, UM982)
#define NEB_CAP_SBAS_TIMEOUT                                                   \
  (1u << 18) // CONFIG SBAS TIMEOUT (Manual §4.10 fn: UM982 only, build-gated)
#define NEB_CAP_MASK (1u << 19) // MASK / UNMASK       (Manual §5: all models)
#define NEB_CAP_MASK_CN0                                                       \
  (1u << 20) // MASK RTCMCN0/CN0    (Manual §5.2 fn: UM982 only, build-gated)
#define NEB_CAP_HEADING_OFFSET                                                 \
  (1u << 21) // CONFIG HEADING OFFSET (Manual §4.9: UM980, UM982)
#define NEB_CAP_SIGNALGROUP                                                    \
  (1u << 22) // CONFIG SIGNALGROUP   (Manual §4.21: UM980, UM982)
#define NEB_CAP_ALGRESET                                                       \
  (1u << 23) // CONFIG ALGRESET      (Manual §4.24: UM960, UM980, UM982)
#define NEB_CAP_ADMIN                                                          \
  (1u << 24) // UNLOG/FRESET/RESET/SAVECONFIG (Manual §8: all models)
#define NEB_CAP_LOGGING                                                        \
  (1u << 25) // data-output commands (Manual §7: all models; per-message
             // support is device-arbitrated)

// Capability set for a given model, derived from the manual. Exposed so tests
// and tooling can reason about capabilities without opening a port.
neb_caps_t neb_caps_for_model(neb_model_t model);

// ---------------------------------------------------------------------------
// Handle.
// ---------------------------------------------------------------------------
typedef struct {
  neb_transport_t transport; // byte layer (real termios, or a test fake)
  serial_t serial;           // backing storage for the real transport's ctx
  neb_model_t model;         // chip model this handle was opened as
  neb_caps_t caps;           // capability flags, set from `model` at open time
  int timeout_ms;            // per-command acknowledgement timeout
  int is_open;               // guard against use-before-open / use-after-close
} neb_handle_t;

// Default acknowledgement timeout if the caller does not override it.
#define NEB_DEFAULT_TIMEOUT_MS 1000

// Open a handle on `device` at `baudrate`, declaring the chip `model`. Wires up
// the real termios transport and populates the capability bitfield from
// `model`. Returns NEB_OK or an error.
neb_status_t neb_open(neb_handle_t *handle, neb_model_t model,
                      const char *device, int baudrate);

// Close the handle and its underlying port. Safe on an already-closed handle.
void neb_close(neb_handle_t *handle);

// True if `handle` has capability `cap`.
int neb_has_cap(const neb_handle_t *handle, neb_caps_t cap);

// ---------------------------------------------------------------------------
// The single command choke point.
// ---------------------------------------------------------------------------
// Frames `command` (an abbreviated-ASCII command WITHOUT line terminator),
// writes it, waits for the "$command,...,response: ..." acknowledgement, and
// maps the outcome to a status:
//   - write fails                    -> NEB_ERR_IO
//   - no ack within timeout          -> NEB_ERR_TIMEOUT
//   - ack says "response: OK"        -> NEB_OK
//   - ack says "PARSING FAILED ..."  -> NEB_ERR_NAK
//   - ack unparseable                -> NEB_ERR_PARSE
//
// If `response` is non-NULL, up to `response_size - 1` bytes of the raw device
// reply (acknowledgement plus any query payload lines that arrive) are copied
// in and NUL-terminated; useful for query commands that return data.
//
// An acknowledgement whose "*hh" checksum does not match is treated as line
// noise and skipped, not as a rejection -- a real ack arriving later in the
// same read still counts.
//
// Every capability-module command wrapper must call through here and must not
// touch the transport directly.
neb_status_t neb_send_command(neb_handle_t *handle, const char *command,
                              char *response, size_t response_size);

// ---------------------------------------------------------------------------
// Raw stream reading.
// ---------------------------------------------------------------------------
// Read bytes straight off the transport, with no command framing and no
// acknowledgement parsing. This is how you read a stream the receiver is
// already producing -- RTCM3 corrections, NMEA, any Manual §7 output message --
// after turning it on with the neb_logging commands.
//
// Waits up to `timeout_ms` for the first byte, then returns whatever has
// arrived; a serial read yields the bytes present, not whole messages, so the
// caller reassembles frames itself.
//
//   - at least one byte read     -> NEB_OK, *received > 0
//   - nothing within the timeout -> NEB_ERR_TIMEOUT, *received == 0
//   - transport failure          -> NEB_ERR_IO
//
// `received` may not be NULL. Do not interleave this with neb_send_command()
// carelessly: bytes belonging to a command's acknowledgement can be swallowed
// here, and vice versa.
neb_status_t neb_read_raw(neb_handle_t *handle, uint8_t *buffer, size_t size,
                          int timeout_ms, size_t *received);

#endif
