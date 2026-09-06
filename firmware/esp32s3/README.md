# ESP32-S3 integration contract

This directory intentionally does not name a board yet. ESP32-S3 plus SX126x is
the reference class, but GPIO, RF switch, oscillator, display, GNSS, power, and
radio timing assumptions must come from the exact hardware revision.

## Adapter responsibilities

The embedded radio-control task must:

1. Own the only SX126x driver instance.
2. Publish the next FLARM window to `RadioArbiter`.
3. Route every Meshtastic RX, CAD, and TX through `MeshtasticGate`.
4. Treat `not_before_us` as the earliest safe hardware start and
   `expires_at_us` as a hard deadline.
5. On FLARM preemption, synchronously abort mesh, mask/drain its IRQs, restore
   the FLARM modem configuration, and only then begin FLARM work.
6. Call `tick` at radio events and before dispatching work; a clock rollback or
   stale FLARM schedule must leave mesh silent.
7. Feed measured worst-case handoff latency back into
   `RadioArbiterConfig::radio_handoff_us` with margin.

## Required platform hooks

- Monotonic 64-bit microsecond clock with documented wrap behavior.
- Single high-priority radio-control task and IRQ-to-task event queue.
- SX126x cancel/standby/reconfigure functions with bounded latency.
- FLARM schedule publisher and completion signal.
- Modified Meshtastic radio interface that accepts denied/truncated RX leases
  and denied TX attempts without bypass or unbounded retry.
- One NimBLE server retaining FLARM service `0xFFE0` and characteristic
  `0xFFE1`, plus any mesh-facing service selected later.
- Display renderer for `SplashFrame`.
- Watchdog and persistent fault counters.

## Bring-up measurements

Do not tune the default guards by intuition. Capture at least:

- mesh abort request to radio standby;
- standby to FLARM configuration complete;
- IRQ queue and task wake-up latency under BLE/display load;
- complete LoRa airtime versus `loraAirtimeUs` for every supported modem preset;
- FLARM deadline jitter during continuous mesh requests; and
- behavior across `micros()` wrap, sleep, reset, and GNSS time corrections.

The portable core uses a 64-bit monotonic clock. Wall-clock or GNSS corrections
must never be passed directly as `now_us`.
