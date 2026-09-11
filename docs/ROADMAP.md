# BONK vNext roadmap

## Phase 0 — control-plane foundation (current)

- [x] Publish a public GPL-3.0 repository.
- [x] Define and test FLARM-first radio leases.
- [x] Bound Meshtastic RX and validate complete TX airtime.
- [x] Preserve the FLARM BLE UUID contract and prioritize telemetry.
- [x] Add a BONK-branded boot-state model.
- [x] Hard-disable direct ADS-B Out RF and document its boundary.
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

## Phase 4 — TABS electronic conspicuity

Goal: make the paraglider electronically visible to nearby aircraft through a
supported external aviation transmitter without turning BONK itself into an
uncertified ADS-B/TABS RF device.

- [x] Select external TABS / approved 1090ES as the U.S. architecture.
- [x] Add a device-neutral `TabsTransport` firmware contract.
- [x] Keep direct ADS-B/UAT/1090ES encoding and RF permanently disabled in BONK.
- [ ] Resolve the lawful identity/configuration path for the intended Part 103
  paraglider operation.
- [ ] Select a currently purchasable lightweight transmitter with documented
  integration interfaces.
- [ ] Select the required pressure-altitude source.
- [ ] Select/validate the required GNSS integrity source.
- [ ] Design the 1090 MHz antenna/feedline/body-shadowing installation.
- [ ] Design an independently protected power path and brownout behavior.
- [ ] Implement the selected manufacturer's serial/control driver.
- [ ] Add simulated fault tests for stale position, invalid identity, encoder
  failure, transport loss, low voltage, and transmitter-disable behavior.
- [ ] Perform bench integration using manufacturer-approved/dummy-load test
  procedures before any radiated testing.

See `docs/TABS_CONSPICUITY.md` for blockers and acceptance criteria.

Exit criterion: a supported external device has a validated identity,
installation, power, antenna, navigation/altitude source, and fail-closed BONK
control path, with the regulatory path documented for the intended operation.

## Phase 5 — hardening and field evaluation

- [ ] Persist fault counters and timing high-water marks.
- [ ] Add brownout, GNSS-loss, BLE-congestion, and clock-fault tests.
- [ ] Conduct shielded RF/coexistence tests before any field transmission.
- [ ] Publish supported hardware, configuration, and rollback instructions.
- [ ] Complete an independent safety and regulatory review.

## Later — traffic receive integrations

- [ ] Evaluate receive-only ADS-B traffic sources separately from transmit
  features.
- [ ] Keep received traffic logically separate from FLARM-originated traffic and
  preserve source/integrity metadata.

The roadmap is ordered. A checked box is not a certification claim.
