# Arduino C++ port

## Why C++

C++ is not universally "more capable" than Python, but it is the correct
integration language here:

- Meshtastic's supported Heltec V3 firmware target is C++ and PlatformIO.
- Arduino/ESP-IDF expose the SX1262, NimBLE, FreeRTOS tasks, interrupts,
  watchdogs, and deep sleep directly.
- Timing and memory behavior can be bounded without a MicroPython interpreter
  or garbage-collection pause.
- The portable BONK safety logic can run unchanged in native CI and firmware.

The tradeoff is more compilation complexity and slower iteration. The preserved
Python version remains the behavioral oracle while the port is validated.

## Preserved behavior

| Working Python behavior | Arduino C++ implementation |
|---|---|
| GPS UART 1, GPIO 4/5, 9600 baud | `gps_serial` with the same UART and pins |
| External FLARM UART 2, GPIO 6/7, 57600 baud | `flarm_serial` with the same UART and pins |
| BLE service/characteristic `FFE0`/`FFE1` | NimBLE notify/write characteristic with the same UUIDs |
| Forward complete GPS and FLARM lines | Fixed-size line readers and FLARM-priority BLE queue |
| Parse `$PFLAU` target count | Bounds-checked portable parser with native tests |
| SSD1306 at `0x3C`, GPIO 17/18 | Adafruit SSD1306 on the same bus and address |
| Battery GPIO 1, multiplier 4.9 | Same transfer function in portable C++ |
| Hold GPIO 0 for two seconds | Wrap-safe timer and ESP32-S3 deep sleep wake on GPIO 0 |
| `AW-S3` screen/advertising name | Deliberately renamed `BONK`; BLE UUID compatibility remains |

Long serial lines are now bounded at 244 bytes. An over-length line is discarded
through its newline instead of risking heap growth or forwarding a truncated
sentence.

## Meshtastic seam

The C++ conversion does not pretend the stock Meshtastic firmware has already
been merged. The onboard SX1262 remains silent in this commit. The next adapter
must route every Meshtastic RX, CAD, and TX request through `MeshtasticGate`,
honor lease deadlines, and synchronously stop mesh when FLARM foreground work
arrives. Only then should `setMeshEnabled(true)` be called.

Official references:

- [PlatformIO Heltec WiFi LoRa 32 V3 board](https://docs.platformio.org/en/latest/boards/espressif32/heltec_wifi_lora_32_V3.html)
- [Meshtastic firmware](https://github.com/meshtastic/firmware)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
