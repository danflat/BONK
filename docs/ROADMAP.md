# BONK vNext roadmap

## Phase 0 — control-plane foundation (current)

- [x] Publish a public GPL-3.0 repository.
- [x] Define and test FLARM-first radio leases.
- [x] Bound Meshtastic RX and validate complete TX airtime.
- [x] Preserve the FLARM BLE UUID contract and prioritize telemetry.
- [x] Add a BONK-branded boot-state model.
- [x] Hard-disable ADS-B Out and document its boundary.
- [x] Preserve the exact working MicroPython firmware as a public baseline.
- [x] Port the working GPS/FLARM/BLE/OLED bridge to Arduino C++.
- [ ] Run the same tests against recorded timing traces from target hardware.

## Phase 1 — select and bring up hardware

- [x] Select the Heltec WiFi LoRa 32 V3 / ESP32-S3 / SX1262 target.
- [x] Record working GPS, external FLARM, OLED, battery, button, and onboard
  SX1262 pins.
- [x] Add a checked-in PlatformIO Arduino target.
- [x] Use the ESP32 monotonic 64-bit microsecond timer in the Arduino adapter.
- [ ] Add the hardware watchdog and persistent reset-reason reporting.
- [ ] Implement SX1262 abort, IRQ drain, and ownership handoff in the
  Meshtastic adapter.
- [x] Implement every splash state in the Heltec V3 display adapter.

Exit criterion: repeatable radio handoff timing, including worst-case latency,
is measured on hardware and fits inside the configured guard.

## Phase 2 — FLARM vertical slice

- [ ] Integrate the chosen FLARM-compatible protocol source with clear license
  and provenance records.
- [ ] Feed GNSS time/position and schedule future FLARM radio windows.
- [ ] Restore BLE telemetry against representative flight apps/instruments.
- [ ] Add golden packet vectors, malformed-input tests, and RF bench tests.
- [ ] Keep mesh disabled until FLARM-only behavior is stable.

Exit criterion: FLARM-only soak tests complete without missed windows or
watchdog resets.

## Phase 3 — opportunistic Meshtastic

- [ ] Pin an upstream Meshtastic firmware revision.
- [ ] Route every SX126x operation through a BONK hardware adapter.
- [ ] Add synchronous cancellation for RX, CAD, and queued TX.
- [ ] Suppress retries that cannot finish before the next FLARM guard.
- [ ] Measure packet delivery versus missed/late FLARM work under stress.

Exit criterion: randomized and hardware-in-the-loop stress tests observe zero
mesh ownership inside protected FLARM intervals.

## Phase 4 — hardening and field evaluation

- [ ] Persist fault counters and timing high-water marks.
- [ ] Add brownout, GNSS-loss, BLE-congestion, and clock-fault tests.
- [ ] Conduct shielded RF/coexistence tests before any field transmission.
- [ ] Publish supported hardware, configuration, and rollback instructions.
- [ ] Complete an independent safety and regulatory review.

## Later — traffic integrations

- [ ] Evaluate receive-only traffic sources separately from transmit features.
- [ ] Define a data-only interface to compliant ADS-B Out equipment if a real
  operational need and lawful installation path exist.
- [ ] Keep all direct ADS-B Out encoding/RF transmission outside BONK firmware.

The roadmap is ordered. A checked box is not a certification claim.
