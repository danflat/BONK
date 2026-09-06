# ESP32-S3 integration contract

The selected target is the Heltec WiFi LoRa 32 V3: ESP32-S3, onboard SX1262,
SSD1306 OLED, and the external GPS/FLARM wiring preserved from the working
MicroPython firmware. `platformio.ini` and `src/heltec_v3_main.cpp` contain the
buildable Arduino C++ port.

See `include/bonk/airwhere_bridge.hpp` for the authoritative pin map. The
onboard SX1262 uses CS 8, SCK 9, MOSI 10, MISO 11, reset 12, busy 13, and DIO1
14, matching Meshtastic's supported `heltec-v3` variant.

## Adapter responsibilities

The embedded radio-control task must:

1. Own the only onboard SX1262 driver instance used by Meshtastic.
2. Treat each external FLARM UART sentence as foreground work and publish
   future FLARM windows if the external unit later exposes usable timing.
3. Route every Meshtastic RX, CAD, and TX through `MeshtasticGate`.
4. Treat `not_before_us` as the earliest safe hardware start and
   `expires_at_us` as a hard deadline.
5. On FLARM foreground activity, synchronously pause mesh and drain its pending
   IRQ work. The external FLARM radio remains on UART 2; it does not use the
   SX1262.
6. Call `tick` at radio events and before dispatching mesh work; a clock
   rollback or stale scheduled window must leave mesh silent.
7. Feed measured worst-case handoff latency back into
   `RadioArbiterConfig::radio_handoff_us` with margin.

## Required platform hooks

- Monotonic 64-bit microsecond clock using `esp_timer_get_time()`.
- Single high-priority radio-control task and IRQ-to-task event queue.
- SX126x cancel/standby/reconfigure functions with bounded latency.
- FLARM UART foreground signal and an optional future-window publisher.
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
