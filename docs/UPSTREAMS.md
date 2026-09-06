# Upstream provenance

The project owner's working `AirWhereS3-danflat.py` firmware is preserved in
`original/` with its SHA-256. The C++ port and initial BONK control-plane code
were written for this repository and do not copy source from the third-party
projects below. They are integration candidates and historical references:

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
