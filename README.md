# BONK

**FLARM-first paragliding safety and mesh firmware for ESP32-S3.**

> [!WARNING]
> BONK vNext is an early control-plane implementation. It has not been validated
> as flight-critical equipment and must not be your only collision-avoidance or
> situational-awareness system.

BONK gives FLARM absolute ownership of time-critical radio work. A modified
Meshtastic integration may use only explicitly granted idle slices and is
preempted before every protected FLARM window. If timing becomes uncertain,
mesh stops.

## What works in this slice

- Deterministic, allocation-free radio arbiter with FLARM preemption.
- Bounded Meshtastic receive slices and airtime-checked transmit leases.
- Semtech LoRa time-on-air calculation for scheduling complete packets.
- FLARM-first BLE transmit queue retaining the legacy `0xFFE0` service and
  `0xFFE1` characteristic UUIDs.
- Board-neutral BONK boot/splash state model.
- Fail-safe handling for missing FLARM schedules and monotonic-clock rollback.
- Native C++17 tests and CI, with no embedded SDK required.

The current slice deliberately stops at the hardware boundary. SX126x radio,
GNSS, display, NimBLE, and upstream Meshtastic adapters still need to be wired
for the selected ESP32-S3 board and tested on hardware.

## Safety invariant

```text
FLARM request -> revoke mesh -> hand off radio -> perform FLARM work
```

Mesh never receives a lease unless its complete operation fits before the next
protected FLARM window. Receive leases may be shortened; transmit leases are
never shortened because cutting a LoRa packet mid-air would still consume the
channel without producing a usable frame.

## Build and test

You need GNU Make and a C++17 compiler:

```sh
make test
make demo
./build/scheduler_demo
```

For a sanitizer pass:

```sh
make test-sanitize
```

## Repository map

- `include/bonk/` — portable public interfaces.
- `src/` — portable scheduler, airtime, BLE queue, and splash logic.
- `tests/` — native behavioral and safety-invariant tests.
- `examples/` — runnable scheduling trace.
- `firmware/esp32s3/` — embedded integration contract and bring-up checklist.
- `docs/ARCHITECTURE.md` — ownership model and timing behavior.
- `docs/ROADMAP.md` — staged vNext plan.
- `docs/ADS_B_OUT.md` — intentionally disabled ADS-B Out integration policy.

## Project boundaries

- BONK does not implement or enable an ADS-B Out RF transmitter. Any future
  output path must hand data to compliant equipment and pass the applicable
  installation and regulatory process.
- This repository does not claim FLARM certification or compatibility beyond
  the explicitly tested interfaces.
- No upstream AirWhere, SoftRF, or Meshtastic source has been copied into this
  initial slice. See `docs/UPSTREAMS.md` before importing code.

Licensed under GPL-3.0. See `LICENSE`.
