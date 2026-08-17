# Hardware verification & compatibility

## What is and isn't covered without hardware

The unit tests (`make test`) verify the library's logic **without any device**:
the exact wire-format string of every command (against the manual), parameter
validation, capability gating, and response parsing (via a mock transport fed
real captured bytes). That's the bulk of what can go wrong, and it runs in CI.

What unit tests **cannot** confirm is that a *real receiver actually accepts a
command and responds as expected* — that a given firmware parses the syntax,
that a model-restricted or build-gated command isn't silently rejected, that the
acknowledgement format matches what the parser expects, and so on. That requires
running against physical hardware (`make test-hardware`).

**We currently have two devices** (a Unicore UM980 and a Holybro UM982 — see the
matrix below). So for the models we don't own — UM960, UM960L — and for every
firmware build other than the ones on hand, the on-device behavior is
**unverified**. The library is written to the manual and should be correct, but
"the manual says X" is not the same as "this board did X."

Two other realities worth stating plainly:

- **Firmware builds vary.** Several commands are gated on a minimum build (e.g.
  PPP needs UM980 Build7923+); the capability check is model-level only and the
  device is the real arbiter. A command that works on one build may `NEB_ERR_NAK`
  on an older one.
- **The same module ships under different integrators.** A UM980/UM982 is often
  mounted on a carrier board from a different vendor (evaluation kits, RTK
  receivers, drone modules, etc.), sometimes with different default configs,
  interfaces, or relabeled firmware. On-device results should record the
  specific integration, not just "UM980."

If you can run this against real hardware — **any** N4-family board, including
different integrators/manufacturers of the same module — please contribute your
results (see below). The goal is a maintained list of what's confirmed working.

## Verification matrix

Levels:
- **accepted** — the device returned `response: OK` for the command's wire
  string, confirming the *syntax* is parsed on that firmware. (Most entries are
  run as effect-free no-ops.)
- **effect** — the command's functional effect was observed, not just acceptance.
- **read** — a read-only query returned and parsed correctly.

| Model  | Integrator / board        | Firmware              | Interface       | Coverage                                                                                       | Contributor            | Date       |
|--------|---------------------------|-----------------------|-----------------|------------------------------------------------------------------------------------------------|------------------------|------------|
| UM980  | (unspecified dev unit)      | R4.10 Build13504 | USB serial 8N1        | Ack/NAK/timeout format & checksum; case-insensitivity; MODE/CONFIG query (read); PPP, serial, NMEA0183, ANTIJAM, AGNSS, RTCMB1CB2a, IONMODE, MMP, EVENT (accepted, mostly no-ops) | project maintainer | 2026-08-15 |
| UM982  | Holybro H-RTK Unicore UM982 | R4.10 Build11826 | USB serial 8N1 (COM3) | MODE/MASK query (read); heading §4.8 mode / reliability / length-with-params / offset, MODE HEADING2, MASK CN0, SBAS TIMEOUT (accepted, RAM-only). Discrepancy: bare `CONFIG HEADING LENGTH` NAKs — see below | project maintainer | 2026-08-17 |

Everything not listed above is **unit-tested only** — correct per the manual,
unconfirmed on silicon.

### UM982 §4.8 heading — verified, with one firmware discrepancy

The UM982-only `neb_heading` §4.8 commands (mode / reliability / length,
`NEB_CAP_HEADING`) plus MODE HEADING2, MASK CN0, and SBAS TIMEOUT were formerly
ALPHA (no UM982 on hand). They are now **hardware-verified** on the Holybro UM982
above via `make test-hardware-um982`, and the ALPHA doc-comments in
`neb_heading.h` have been lifted.

One discrepancy is recorded: bare **`CONFIG HEADING LENGTH`**
(`neb_heading_length_default`) is **rejected** by R4.10Build11826 with
`PARSING FAILD GRAMMAR ERROR`, even though Manual §4.8 Table 4-13 documents
omitting both parameters to use the default configuration. Prefer the
parameterized `neb_heading_length` (which the device accepts). The builder is
kept — it is faithful to the manual — but carries a hardware note, and the HIL
test pins the observed `NEB_ERR_NAK` so a future firmware that accepts it will
flip the test and prompt a revisit.

Still un-owned and unverifiable here: **UM960** and **UM960L**. If you have one,
running against it and reporting results is especially valuable.

## How to contribute a hardware run

1. Connect the receiver and note: exact **model**, **integrator/board**,
   **firmware** (`VERSION` / the `#VERSION` log), and the **interface/baud**.
2. Build and run the integration test against your port:
   ```sh
   NEB_TEST_PORT=/dev/ttyUSB0 make test-hardware         # UM980, read-only
   NEB_TEST_PORT=/dev/ttyUSB0 make test-hardware-um982   # UM982, RAM-only
   ```
   The UM980 harness (`tests/integration/test_hil_um980.c`) runs only read-only
   queries. The UM982 harness (`tests/integration/test_hil_um982.c`) also sends
   **RAM-only** config commands (values at or near the device defaults) to
   confirm the device accepts them. Neither ever sends `SAVECONFIG` or `FRESET`,
   so a **power-cycle restores** whatever config was saved before the run; and
   neither runs without an explicit port.
3. To cover more commands, add them to a copy of the integration test — but
   **only commands that are read-only or verifiably non-mutating for your setup**
   (a set that matches the device's current value is an effect-free no-op). When
   in doubt, ask in a PR before sending it. Never send `SAVECONFIG` blindly.
4. Report results by adding a row (or refining one) in the matrix above via PR,
   with your coverage level and what you observed. If a command was **rejected**
   where the manual says it should work, note the model + firmware — that's a
   build-gating or manual-accuracy finding worth capturing.

Captured real responses (ack lines, error strings) are also valuable as test
fixtures — see `tests/unit/fixtures/um980_session.h` for the format.
