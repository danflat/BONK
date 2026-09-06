# BONK vNext roadmap

## Phase 0 — control-plane foundation (current)

- [x] Publish a public GPL-3.0 repository.
- [x] Define and test FLARM-first radio leases.
- [x] Bound Meshtastic RX and validate complete TX airtime.
- [x] Preserve the FLARM BLE UUID contract and prioritize telemetry.
- [x] Add a BONK-branded boot-state model.
- [x] Hard-disable ADS-B Out and document its boundary.
- [ ] Run the same tests against recorded timing traces from target hardware.

## Phase 1 — select and bring up hardware

- [ ] Record the exact ESP32-S3 board revision, SX126x part, oscillator, RF
  switch, TCXO, GNSS, display, and power pins.
- [ ] Add a checked-in PlatformIO or ESP-IDF target after board selection.
- [ ] Implement the monotonic microsecond clock and watchdog adapters.
- [ ] Implement SX126x abort, IRQ drain, retune, and ownership handoff.
- [ ] Render every splash state on the real display.

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
