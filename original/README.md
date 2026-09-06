# Original working AirWhere S3 baseline

`AirWhereS3-danflat.py` is the unmodified working MicroPython firmware supplied
by the project owner. It is retained as the behavioral and hardware baseline
for BONK's Arduino C++ port.

- Source SHA-256: `ff47dbd6e3ac19ec90ce6d858a1197347b435ce690008c34d605c206acf032e7`
- Target: Heltec WiFi LoRa 32 V3 / ESP32-S3
- GPS UART: RX GPIO 4, TX GPIO 5, 9600 baud
- External FLARM UART: RX GPIO 6, TX GPIO 7, 57600 baud
- FLARM-compatible BLE service: `0xFFE0`
- FLARM-compatible BLE characteristic: `0xFFE1`
- OLED: SDA GPIO 17, SCL GPIO 18, reset GPIO 21, Vext GPIO 36
- Battery ADC: GPIO 1, ADC control GPIO 37
- Power button: GPIO 0, hold for two seconds to shut down

Do not edit the baseline file. Behavioral changes belong in the C++ port and
must be compared against this version.
