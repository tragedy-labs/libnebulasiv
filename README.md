# libnebulasiv

A modular C library for configuring **Unicore NebulasIV (N4)** high-precision RTK
GNSS receivers — **UM980, UM982, UM960, and UM960L**.

It turns the receivers' abbreviated-ASCII command set into a small, typed C API:
you call a function, it validates the parameters, builds the exact wire string,
sends it through a pluggable transport, and returns a `neb_status_t` reflecting
what the device actually acknowledged.

> **Status: early development.** The command/configuration layer is implemented
> and unit-tested against the manual, with a growing subset verified on real
> hardware (see [HARDWARE_TESTING.md](HARDWARE_TESTING.md)). Parsing of the
> receivers' *output* messages (position / heading records) is not yet
> implemented — today the library builds and sends commands and interprets their
> acknowledgements.

## Design

- **Capability modules, not chip modules.** One module per functional area
  (`mode`, `config`, `rtk`, `mask`, `heading`, `assist`, `admin`, `logging`) —
  shared across all chips rather than duplicated per model.
- **Thin per-chip bundle headers.** `um980.h`, `um982.h`, `um960.h`, `um960l.h`
  simply include the capability modules that model supports.
- **One choke point.** Every command funnels through a single
  `neb_send_command()`; every command returns `neb_status_t`.
- **Capability gating.** A `neb_caps_t` bitfield is set from the chip model at
  open time; commands a model doesn't support are rejected with
  `NEB_ERR_UNSUPPORTED` before anything hits the wire. Build-gated commands
  (e.g. UM982 `MASK CN0`) are gated per value.
- **build/send split.** Pure `neb_build_*()` string builders (no I/O) plus thin
  wrappers that gate, build, and send — so every wire string is unit-testable
  without a device.
- **Pluggable transport.** A small vtable (`write`/`read`/`ctx`); a POSIX
  `termios` serial backend is included, and tests drive a mock. The serial
  backend claims the port exclusively — a second opener gets `EBUSY` rather
  than silently splitting the byte stream — and restores the port's previous
  line settings on close.
- **Injection-safe validation.** Free-text fields (message names, antenna
  names/serials) are validated against control-character / quote / token
  injection before being framed.

Every command builder cites its manual section, and the wire strings are pinned
by exact-string unit tests. See [ARCHITECTURE.md](ARCHITECTURE.md) for the full
rationale.

## Coverage

Implemented, per the *Unicore Reference Commands Manual for N4 (V2 R1.1)*:

- **MODE** (§3) — base / rover / heading operating modes
- **CONFIG** (§4) — PPP, RTK, SBAS, base antenna, PPS, dual-antenna heading,
  signal groups, anti-jam, and more
- **Signal masks** (§5) — elevation / PRN / frequency / C/N0
- **Assisted GNSS input** (§6) — `AIDPOS` / `AIDTIME`
- **Data output** (§7) — turning messages on/off (periodic / once / on-change),
  with a typed enum for the standard NMEA messages
- **Admin** (§8) — `UNLOG`, `RESET`, `FRESET`, `SAVECONFIG`

Not yet implemented: parsing of output-message payloads into structs. Out of
scope by design: `AUTHCODE` (one-time vendor provisioning).

## Building

Requires a C11 compiler.

```sh
# Library
cmake -S . -B build && cmake --build build

# Unit tests — no hardware needed (vendored Unity, ASan/UBSan, -Werror)
make test

# Hardware-in-the-loop (opt-in; needs a real receiver at $NEB_TEST_PORT).
# One suite for every model: it identifies the receiver and adapts.
NEB_TEST_PORT=/dev/ttyUSB0 NEB_TEST_BOARD="my board" make test-hardware
NEB_TEST_LEVEL=read NEB_TEST_PORT=/dev/ttyUSB0 make test-hardware  # queries only
```

## Usage

```c
#include "um980.h"   // pulls in exactly the modules the UM980 supports

neb_handle_t gps;
if (neb_open(&gps, NEB_MODEL_UM980, "/dev/ttyUSB0", 115200) != NEB_OK)
    return 1;

char mode[256];
if (neb_mode_query(&gps, mode, sizeof mode) == NEB_OK)
    printf("%s\n", mode);

neb_status_t st = neb_config_ppp_enable(&gps, NEB_PPP_B2B);
if (st != NEB_OK)
    fprintf(stderr, "PPP enable: %s\n", neb_strerror(st));

neb_close(&gps);
```

## RTK base station

A ready-to-run base station built on this library lives in its own repository,
[nebulasiv_base_station](https://github.com/tragedy-labs/nebulasiv_base_station):
`neb_base` surveys in a receiver's position, turns on the RTCM3 correction
messages a rover needs, and streams them out as MAVLink `GPS_RTCM_DATA` (#233)
over UDP for QGroundControl / PX4 / ArduPilot. It is also the worked example of
driving this library end to end.

## Safety

The library **never sends `SAVECONFIG` or `FRESET` on your behalf.**
Configuration changes are live-only (RAM) and revert on power-cycle unless you
choose to persist them. The hardware test suite follows the same discipline —
see [HARDWARE_TESTING.md](HARDWARE_TESTING.md).

## Hardware verification

On-device results are tracked per model / integrator / firmware build in
[HARDWARE_TESTING.md](HARDWARE_TESTING.md). One suite covers every model: it
identifies the attached receiver and adapts, recording each run to
`tests/results/`, from which the matrix is generated (`make hardware-matrix`).
A command the attached model does not support is verified as *refused* rather
than skipped, so even a board with a small feature set produces a full row.

Contributions from other N4 boards — especially UM960 / UM960L, which the
maintainer does not have — are welcome.

## License

libnebulasiv is licensed under the **GNU General Public License v3.0** — see
[LICENSE](LICENSE). The vendored Unity test framework under `tests/unity/` is
covered by its own MIT license (`tests/unity/LICENSE.txt`).
