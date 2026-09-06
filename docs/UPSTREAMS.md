# Upstream provenance

The initial BONK vNext control-plane code was written for this repository and
does not copy source from the projects below. They are relevant integration
candidates and historical references:

| Project | Relevance | License observed during initial review |
|---|---|---|
| [AirWhere-esp32](https://github.com/PhilColbert/AirWhere-esp32) | Historical AirWhere firmware and BLE behavior | GPL-3.0 |
| [SoftRF](https://github.com/lyusupov/SoftRF) | Current traffic-protocol and ESP32 hardware reference | GPL-3.0 |
| [Meshtastic firmware](https://github.com/meshtastic/firmware) | Mesh stack to adapt behind the BONK gate | GPL-3.0 |

Before importing any upstream code:

- pin the exact commit;
- preserve its notices and authorship;
- document copied and modified paths;
- review compatibility with BONK's GPL-3.0 license; and
- keep functional provenance separate from claims of certification or approval.
