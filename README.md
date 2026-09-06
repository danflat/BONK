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

## Original baseline

The exact working MicroPython firmware is preserved unchanged at
[`original/AirWhereS3-danflat.py`](original/AirWhereS3-danflat.py). Its recorded
SHA-256 is `ff47dbd6e3ac19ec90ce6d858a1197347b435ce690008c34d605c206acf032e7`.
The permanent comparison branch is
[`archive/airwhere-s3-working-baseline`](https://github.com/danflat/BONK/tree/archive/airwhere-s3-working-baseline).

## What works in this slice

- Buildable Arduino C++ port of the working Heltec V3 flight bridge.
- The original GPS and external-FLARM UART pins, speeds, forwarding behavior,
  OLED, battery monitor, and two-second shutdown interaction.
- Deterministic, allocation-free radio arbiter with FLARM preemption.
- Bounded Meshtastic receive slices and airtime-checked transmit leases.
- Semtech LoRa time-on-air calculation for scheduling complete packets.
- FLARM-first BLE transmit queue retaining the legacy `0xFFE0` service and
  `0xFFE1` characteristic UUIDs.
- Board-neutral BONK boot/splash state model.
- Fail-safe handling for missing FLARM schedules and monotonic-clock rollback.
- Native C++17 tests plus a PlatformIO Heltec V3 build in CI.

The Arduino port now owns the Heltec V3 hardware bridge, NimBLE server, and
display. The external FLARM device remains on UART 2 exactly as in the working
version. The onboard SX1262 is reserved for the next step: routing Meshtastic's
radio operations through `MeshtasticGate`. That adapter is not enabled yet.

## Safety invariant

```text
FLARM activity -> suspend mesh -> service FLARM -> resume only when safe
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

Build the flashable Heltec WiFi LoRa 32 V3 image with PlatformIO:

```sh
pio run -e heltec-v3
pio run -e heltec-v3 -t upload
pio device monitor -b 115200
```

## Repository map

- `include/bonk/` — portable public interfaces.
- `src/` — portable scheduler, airtime, BLE queue, and splash logic.
- `tests/` — native behavioral and safety-invariant tests.
- `examples/` — runnable scheduling trace.
- `firmware/esp32s3/` — embedded integration contract and bring-up checklist.
- `original/` — immutable working MicroPython baseline and provenance.
- `platformio.ini` / `src/heltec_v3_main.cpp` — Arduino C++ firmware target.
- `docs/ARCHITECTURE.md` — ownership model and timing behavior.
- `docs/ROADMAP.md` — staged vNext plan.
- `docs/CPP_PORT.md` — Python-to-Arduino behavior map and tradeoffs.
- `docs/ADS_B_OUT.md` — intentionally disabled ADS-B Out integration policy.

## Project boundaries

- BONK does not implement or enable an ADS-B Out RF transmitter. Any future
  output path must hand data to compliant equipment and pass the applicable
  installation and regulatory process.
- This repository does not claim FLARM certification or compatibility beyond
  the explicitly tested interfaces.
- The owner's original AirWhere S3 file is preserved verbatim. No third-party
  AirWhere, SoftRF, or Meshtastic source has been copied into BONK. See
  `docs/UPSTREAMS.md` before importing code.

Licensed under GPL-3.0. See `LICENSE`.
