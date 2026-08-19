# libnebulasiv architecture

A modular C library for Unicore **NebulasIV (N4)** high-precision GNSS
receivers: UM960, UM960L, UM980, UM982. A consumer includes a single per-chip
header (e.g. `um980.h`) and gets exactly the commands that chip supports, while
all command logic lives once per capability area — never duplicated per chip.

> Status: build in progress, delivered as per-module batches.
> - **`neb_mode`** (§3): complete — base (fixed geodetic *and* ECEF / auto / id
>   / self-optimize), rover (default + profiles), heading2.
> - **`neb_config`** (§4): general commands largely done — query, PPS (§4.3),
>   serial port (§4.2), undulation (§4.4), EVENT (§4.11), smooth (§4.12), MMP
>   (§4.13), NMEA version (§4.14), RTCM B1C/B2a (§4.15), RTCMPHASERATE (§4.16),
>   PSRVELDRPOS (§4.17), AGNSS (§4.18), PPP (§4.19), ANTIJAM (§4.20), IONMODE
>   (§4.25). Deferred (need decisions/careful handling): SIGNALGROUP (§4.21,
>   persistent+resets), ALGRESET (§4.24), antenna/authcode (§4.22–4.23).
>   Correction-stream commands (DGPS §4.5, RTK §4.6, SBAS §4.10, STANDALONE
>   §4.7, base-antenna §4.26) belong to the upcoming `neb_rtk` module.
> - **`neb_rtk`** (§4.5-§4.7, §4.10, §4.26): complete — DGPS (§4.5), RTK
>   timeout/reliability/solution-control (§4.6), STANDALONE (§4.7), SBAS
>   (§4.10), base-antenna model (§4.26).
> - **`neb_mask`** (§5): complete — query, constellation/frequency/elevation/
>   per-satellite mask & unmask, and the UM982-only RTCMCN0/CN0 filters.
> - **`neb_assist`** (§6): complete — assisted position ($AIDPOS) and time
>   ($AIDTIME) AGNSS input (UM980/UM982).
> - **`neb_heading`** (§4.8-4.9): complete — heading mode/reliability/length
>   (§4.8, **UM982-only, ALPHA/hardware-unverified**) and heading/pitch offset
>   (§4.9, UM980/UM982, hardware-checked). See the ALPHA banner in the header.
>   Also covers SIGNALGROUP (§4.21), ANTENNADELTAHEN (§4.22, in `neb_rtk`), and
>   ALGRESET (§4.24, per-value model gating). **§4 is complete**; AUTHCODE (§4.23)
>   is intentionally out of scope (see below).
> - **`neb_admin`** (§8): complete — UNLOG (§8.1, benign), FRESET (§8.2), RESET
>   (§8.3, target-flags bitfield), SAVECONFIG (§8.4). The last three are
>   destructive and carry WARNING doc comments; only UNLOG is hardware-checked.
> - **`neb_logging`** (§7): the data-output *command* mechanism — request any
>   message once / periodically / ONCHANGED, on the current or a given port
>   (`<message> [port] [period|ONCHANGED]`). Two layers: a generic string API for
>   any message, plus a **typed `neb_nmea_message_t`** enum + `neb_logging_nmea_*`
>   wrappers covering §7.1 standard NMEA (14) and §7.2 slave-antenna variants (9).
>   Disable via UNLOG (neb_admin). **Still deferred in §7**: a typed enum for the
>   §7.3 Unicore ASCII/binary logs (~80), and field-level parsing of any output
>   message (commands now, parsers later — see the neb_logging.h header).
> - Every command-*input* chapter (§3-§8) is now covered, except the
>   intentionally-excluded AUTHCODE (§4.23). Deferred within §7: the typed
>   message-name vocabulary and output-message parsing.

### Intentionally out of scope

- **AUTHCODE (§4.23)** — adding a feature-authorization code. Deliberately not
  implemented. It is a one-time vendor-provisioning step (Unicore issues the
  code to unlock features), not a runtime interaction, so it does not fit a
  library meant for operating the receiver. It is also uniquely hazardous: the
  code is saved permanently, **survives FRESET**, and a *wrong* code makes the
  receiver "work abnormally" — with no safe value to test against. A consumer who
  genuinely needs to apply one can paste the `CONFIG AUTHCODE <string>` line into
  a serial terminal once; that deliberate, manual path is safer than a
  convenience wrapper.
>
> The structure below is the template every remaining command follows.

## Layers

```
        um960.h  um960l.h  um980.h  um982.h        <- per-chip bundle headers
                       (aggregate only)
                            |
   neb_mode.*   neb_config.*   (neb_rtk.*, neb_heading.*, ...)  <- capability modules
                            |
                        neb_core.*                  <- handle, send-command choke point
                            |
                         serial.*                   <- POSIX termios byte transport
```

### `serial.c/.h` — transport
POSIX termios open/close/read/write at 8N1. Adds `serial_read_timed()`
(poll-based bounded wait) and `serial_flush_input()`, which `neb_core` needs for
command timeouts and to clear stale bytes before each command. Nothing above
`neb_core` calls this layer directly.

### `neb_core.c/.h` — context + the single command choke point
Owns:
- `neb_t` handle: transport, chip `model`, capability bitfield, ack timeout.
- `neb_status_t`: the status enum **every** command function returns.
- `neb_open()` / `neb_close()`, and `neb_strerror()` for human-readable errors.
- **`neb_send_command()`** — the one function that touches the wire. It frames a
  command (append CR+LF; abbreviated ASCII has no CRC), writes it, scans the
  reply for the `$command,…,response: …` acknowledgement, and maps the outcome
  to a status. Every capability-module builder funnels through here and only
  formats a string — it never touches `serial.*`.

### `neb_protocol.h` — single source of truth for wire strings
All literal command tokens (`"MODE"`, `"CONFIG"`, `"PPP"`, `"Enable"`, …) and
the response markers live here as macros, plus closed-vocabulary value enums
(e.g. `neb_ppp_mode_t`) each with a `static inline` enum→string function. If
Unicore changes a command's casing/spelling in a firmware revision, this file —
and only this file — should need editing. Command builders must not embed raw
literals.

### `neb_mode.c/.h`, `neb_config.c/.h`, … — capability modules
One `.c/.h` pair per capability area (mirroring the manual's own chapter split:
§3 MODE, §4 CONFIG, and later RTK, heading, logging). One function per distinct
command. No command logic is duplicated across chips — a UM980 and a UM982 call
the *same* `neb_config_ppp_enable()`.

### `um960.h` / `um960l.h` / `um980.h` / `um982.h` — bundle headers
Thin aggregators: a set of `#include`s for the capability headers that model
supports, plus a `UM980_MODEL`-style constant. **No function bodies.** They
express intent; the capability bitfield (below) does the actual enforcement.

## The capability bitfield — the real enforcement layer

C won't stop a caller from `#include`-ing a header for a command their chip
lacks, so a runtime bitfield is the enforcement point.

- `neb_caps_t` is a `uint32_t` of `NEB_CAP_*` flags (`NEB_CAP_PPP`,
  `NEB_CAP_HEADING`, …).
- `neb_caps_for_model()` maps each chip model to its flag set. **Every flag's
  model set is taken from the manual's per-command "Applicable to:" line** — not
  from assumptions about chip families, which don't hold here (e.g. `CONFIG
  ANTIJAM` is UM960/UM980 but *not* UM982; `CONFIG HEADING` is UM982-only).
- `neb_open()` sets `handle->caps` from the model.
- **Every** capability-module command checks its flag at entry via
  `neb_has_cap()` and returns `NEB_ERR_UNSUPPORTED` immediately, before touching
  the wire — otherwise the hardware would just NAK it (or, for some inputs,
  silently ignore it).

## Error handling model

`neb_status_t`, returned everywhere, distinguishes:

| Status                   | Meaning                                              |
|--------------------------|------------------------------------------------------|
| `NEB_OK`                 | device replied `response: OK`                        |
| `NEB_ERR_IO`             | serial write/read failure                            |
| `NEB_ERR_TIMEOUT`        | no acknowledgement within `timeout_ms`               |
| `NEB_ERR_NAK`            | device rejected it (`PARSING FAILED …`)              |
| `NEB_ERR_PARSE`          | a reply arrived but couldn't be interpreted          |
| `NEB_ERR_UNSUPPORTED`    | command not supported by this model (cap check)      |
| `NEB_ERR_INVALID_PARAM`  | a parameter failed validation *before* being sent    |
| `NEB_ERR_INVALID_HANDLE` | NULL / unopened handle                               |
| `NEB_ERR_OVERFLOW`       | formatted command exceeded the internal buffer       |

Parameter ranges are validated where the manual specifies them (e.g. `MODE
BASE` geodetic bounds) and rejected with `NEB_ERR_INVALID_PARAM` before hitting
the wire. Where the manual gives no numeric range (e.g. `PPP CONVERGE`
thresholds), we only reject physically-meaningless values and let the device
validate the rest — we do not invent bounds.

## The command / response protocol (verified on hardware)

The N4 manual documents command *input* syntax and output *log* formats but not
the command acknowledgement line. It was captured from a live UM980:

- **Success:** `$command,<echoed command>,response: OK*hh`
- **Rejected:** `$command,<echoed command>,response: PARSING FAILED <reason>,*hh`
- `*hh` = XOR of every character from `$` through the char before `*`,
  **including** the `$` (a variant of the NMEA checksum, which excludes `$`).
- Stray bytes can precede the reply, so the parser scans for the `$command,`
  prefix rather than assuming it starts at offset 0.

## Adding a new command

Every command is two functions: a pure **builder** and a thin **wrapper**.

1. If it needs new wire tokens or a new closed-vocabulary value, add them to
   **`neb_protocol.h`** (token macros; an enum + `static inline` string fn).
2. Add a pure builder `neb_build_<area>_<name>(char *out, size_t len, ...)` to
   the capability module. It validates parameters against the manual's
   documented bounds (returning `NEB_ERR_INVALID_PARAM`), `snprintf`s the exact
   command from `neb_protocol.h` tokens, and returns `NEB_ERR_OVERFLOW` if it
   would not fit. No handle, no I/O. Put a `// Manual §x.y <Command>` citation
   above it.
3. Add the wrapper `neb_<area>_<name>(neb_handle_t *h, ...)`: check the
   capability flag → `neb_build_*` into a `char cmd[NEB_CMD_BUF_LEN]` → on
   success `neb_send_command()`.
4. If the command needs a capability not yet modelled, add a `NEB_CAP_*` flag in
   `neb_core.h` and update `neb_caps_for_model()` from the manual's "Applicable
   to:" line.
5. **Write the test in the same pass** (see Testing). The command is not "done"
   until its manual-as-spec test exists.

## Adding a new chip variant

1. Add a `NEB_MODEL_*` enum value in `neb_core.h`.
2. Add its capability set to `neb_caps_for_model()`, reading each command's
   "Applicable to:" line in the manual.
3. Create a `um<model>.h` bundle header that `#include`s the capability headers
   that model supports (and a `UM<model>_MODEL` constant).

No command logic changes — capability modules are shared.

## Build

```sh
cmake -S . -B build && cmake --build build     # -> build/libnebulasiv.a
cmake --install build --prefix /usr/local      # headers, lib, package files
```

`CMakeLists.txt` builds one target: a static `libnebulasiv.a` from the
capability modules + transport. Applications live outside this repository —
see the `nebulasiv_base_station` repo for a full RTK base station built on it.

Installation exports a `nebulasiv::nebulasiv` target (`find_package(nebulasiv)`)
plus a `pkg-config` file. Both locate their paths relative to where they are
installed rather than where they were configured, so a staged or relocated tree
still resolves. A consumer that vendors the source instead links the same
aliased target, so the two routes are interchangeable; `NEBULASIV_INSTALL`
defaults off when the project is pulled in via `add_subdirectory()`, so a
consumer does not inherit our install rules.

## Testing

Tests use **vendored Unity** (`tests/unity/`, source only — no Ceedling, no
CMock, no Ruby). The runner, Makefile, and transport fake are all hand-written.

```sh
make test            # unit tests only — CI-safe, no hardware
make test-hardware   # integration tests against a real UM980 (opt-in)
make test-all
```

`make test` compiles our code with `-Wall -Wextra -Wpedantic -Werror` under
AddressSanitizer + UndefinedBehaviorSanitizer, so an overrun in a fixed-size
command buffer fails the run. It never needs hardware. `make test-hardware`
requires `NEB_TEST_PORT` (e.g. `NEB_TEST_PORT=/dev/ttyUSB0`) and refuses to run
without it.

### The unit / integration split

- **`tests/unit/`** — no device required. Because the command layer is split
  into pure builders and a mockable transport, *everything* is testable here,
  including UM982-only commands we can't run on a UM980 (a builder just produces
  a string). Layout:
  - `test_neb_<module>_build.c` — builder tests: the exact wire string per the
    manual, every enum token, parameter bounds, invalid-parameter rejection.
  - `test_neb_send_command.c` — response parsing via the mock transport (OK,
    NAK, timeout, garbage, truncation, chunking, I/O failure).
  - `test_neb_caps.c` — capability gating for every model-restricted command.
  - `mock_transport.c` — the hand-written fake `neb_transport_t`.
  - `fixtures/um980_session.h` — real bytes captured from an actual UM980,
    replayed as canned responses.
- **`tests/integration/test_hil.c`** — one hardware suite for every model. It
  identifies the attached receiver with `VERSIONA` and decides per test what
  that model and firmware build should do, so a test has three meaningful
  outcomes: accepted (or a pinned discrepancy), *unsupported* — the capability
  gate refused it, which verifies `neb_caps_for_model()` against real silicon —
  or skipped because the firmware build is below the documented minimum. Never
  `SAVECONFIG` or `FRESET`; `NEB_TEST_LEVEL=read` restricts it to queries.
- **`tests/integration/hil_device.c`** — identification and result recording.
- **`tests/results/*.tsv`** — one file per (model, build, board), written by
  each run. These are the record of what was verified on what; the matrix in
  `HARDWARE_TESTING.md` is generated from them by
  `tools/gen_hardware_matrix.awk` (`make hardware-matrix`) and is not edited by
  hand.

> On-device coverage is limited: models and firmware builds nobody has run the
> suite against are **unverified on silicon** — the library is written to the
> manual, but only unit-tested for those. See **`HARDWARE_TESTING.md`** for the
> verification matrix and how to contribute a hardware run (including different
> integrators of the same module).

### Manual-as-spec convention

A builder test pins the **exact** wire string, not a fuzzy match — the manual is
the specification. For example (Manual §4.19 CONFIG PPP):

```c
// Manual §4.19, Table 4-24 -- PPP enable, every correction-source token
BUILD_OK(neb_build_config_ppp_enable(buf, sizeof(buf), NEB_PPP_B2B),
         "CONFIG PPP Enable B2b-PPP");
BUILD_OK(neb_build_config_ppp_enable(buf, sizeof(buf), NEB_PPP_SSR_RX),
         "CONFIG PPP Enable SSR-RX");
BUILD_ERR(neb_build_config_ppp_enable(buf, sizeof(buf), (neb_ppp_mode_t)999),
          NEB_ERR_INVALID_PARAM);
```

Cite the manual section above each test or group, same as the implementation.
Never invent an expected string, range, enum token, or error behavior — pull it
from the manual. If the manual doesn't specify a case (e.g. out-of-range NAK
behavior), mark it `TEST_IGNORE` with a comment and raise it, rather than
guessing.

### Adding a test for a new command

In the module's `test_neb_<module>_build.c`, add a test that asserts the exact
string for a representative case, one assertion per enum value, integer params
at min/max/mid, and at least one invalid value → `NEB_ERR_INVALID_PARAM`. If the
command is model-restricted, add supported- and unsupported-model cases to
`test_neb_caps.c`. Register the test in the file's `run_*_tests()`.

Style: `.clang-format` (LLVM) formats the tree; `clang-tidy` is available for
static checks but does not gate the build.
