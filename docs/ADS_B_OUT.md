# ADS-B / TABS Out policy

BONK does **not** implement or enable an ADS-B/TABS RF transmitter on the
ESP32-S3/SX1262 hardware.

`include/bonk/features.hpp` pins `BONK_ENABLE_ADSB_OUT` to zero with a
compile-time assertion. The repository contains no 1090ES or UAT modulator, no
ADS-B packet encoder intended for over-the-air transmission, and no software
path that turns the Heltec board into aviation surveillance RF equipment.

## U.S. project direction

The preferred U.S. electronic-conspicuity path is an **external TSO-C199 Traffic
Awareness Beacon System (TABS)** or other appropriately approved 1090ES unit.
FAA guidance describes TABS as voluntary equipage for aircraft otherwise exempt
from transponder/ADS-B requirements; TABS can make the aircraft electronically
visible to airborne collision-avoidance systems, traffic advisory systems, and
ADS-B In receivers.

That purpose aligns with BONK's mission better than attempting to duplicate a
full conventional GA ADS-B Out installation inside hobby firmware.

See [`TABS_CONSPICUITY.md`](TABS_CONSPICUITY.md) for the candidate hardware,
blockers, and acceptance criteria.

## Boundary

BONK may integrate with external equipment through a documented serial/control
interface. That integration must include:

1. A named jurisdiction and intended vehicle/operation.
2. A supported external device with current approval/interface documentation.
3. A valid, non-invented identity/configuration path.
4. Device-owned RF generation, squitter timing, pressure-altitude handling, and
   required navigation-integrity logic.
5. Stale-data rejection, health/status propagation, explicit transmitter
   enable/disable, and a loss-of-link safe state.
6. Separate power/RF/antenna design from the FLARM and Meshtastic radios.

The device-neutral firmware contract is
[`include/bonk/tabs_interface.hpp`](../include/bonk/tabs_interface.hpp).

## Important distinction

TABS voluntary equipage is not the same thing as a rule-compliant ADS-B Out
installation for airspace where 14 CFR 91.225/91.227 compliance is required.
Portable/transmitting aviation equipment outside the TSO-C199 exception must
not be treated as legal merely because it can produce a technically valid
signal.

Receive-only ADS-B traffic ingestion remains a separate possible feature and
must not be described as ADS-B Out.
