# BONK electronic conspicuity: TABS path

BONK's goal is to make paragliders and similar slow, lightweight aircraft more
visible to nearby powered aircraft without turning the ESP32/SX1262 board into
an uncertified aviation transmitter.

## Decision

For U.S. operations, the preferred architecture is an **external Traffic
Awareness Beacon System (TABS)** or other appropriately approved 1090ES device.
BONK remains the flight computer/control plane; the external unit owns all
aviation RF behavior.

This keeps three separate safety domains:

1. **FLARM** — collision-awareness traffic link and BLE forwarding.
2. **Meshtastic** — background, opportunistic mesh on the onboard SX1262.
3. **TABS / 1090ES** — independent external aviation transmitter.

The third domain never shares the SX1262, FLARM radio schedule, packet encoder,
or antenna.

## Why TABS fits the mission

FAA AC 90-114C describes TABS as an option for aircraft exempt from mandatory
transponder/ADS-B equipage. TABS-equipped aircraft can be electronically visible
to airborne collision-avoidance systems, traffic advisory systems, and ADS-B In
receivers. That directly matches BONK's safety objective: improve detectability
to nearby aircraft.

TABS is voluntary electronic conspicuity; it is not a substitute for a
rule-compliant ADS-B Out installation where 14 CFR 91.225/91.227 compliance is
required.

## BONK boundary

BONK may:

- power and monitor a supported external unit;
- exchange control/status messages over a documented serial interface;
- provide ownship state only when the selected external device explicitly
  permits that architecture;
- display transmitter health/fault state;
- fail closed when identity, navigation integrity, pressure altitude, device
  health, or authorization is invalid.

BONK may not:

- generate 1090ES or UAT RF;
- implement a home-built ADS-B/TABS modulator;
- invent or silently assign an aviation identity;
- claim compliance on behalf of an external device or installation;
- transmit when the selected device/jurisdiction/identity configuration has not
  been positively validated.

## Candidate hardware

### Urban Canyon UC100 TABS

The UC100 is the closest architectural fit found so far:

- TSO-C199 Class A design target;
- 1090 MHz;
- approximately 60 g;
- approximately 1 W class typical operating power;
- 10-38 V supply;
- RS-232 system integration;
- dedicated fast transmitter suppression/enable input.

Current blocker: the manufacturer still lists the UC100 as **Coming Soon**.
Treat it as the preferred reference architecture, not purchasable production
hardware, until availability and current approval paperwork are confirmed.

### Trig TT23G

The TT23G is a production, certified 1090ES/Mode S path with TSO-C199 listed by
the manufacturer and is attractive for a proof-of-concept on a test airframe.
It is, however, much heavier and more power-hungry than the UC100-style target:
roughly 315 g for the transponder box plus a 90 g TC20 control head, before
battery, antenna, cabling, and mounts.

### uAvionix ping200X family

The ping200X family is a certified 1090ES transponder architecture with remote
control options and external GPS integration on some variants. It is useful as
an integration/reference candidate, but current product configuration, weight,
GPS requirements, and intended installation class must be checked for the exact
model before selecting it for a foot-launched system.

## Project blockers

### B1 — legal/identity path for a Part 103 paraglider

This is now the primary regulatory blocker rather than a generic open question.
FAA ADS-B guidance states that a valid ICAO/Mode-S aircraft address is required
for normal ADS-B operation and that those addresses are assigned to aircraft.
Urban Canyon's UC10x installation documentation likewise defines the Mode-S
address as a 24-bit number issued by the aircraft registration authority.

A normal Part 103 paraglider is operated as an ultralight vehicle without an
N-number/aircraft registration, so BONK cannot derive or invent a lawful 24-bit
address from the existing operating status.

At the same time, FAA AC 90-114C explicitly identifies TABS as voluntary
electronic-conspicuity equipment for otherwise exempt aircraft. The unresolved
question is therefore very narrow: **what identity/configuration path, if any,
does the FAA and selected TABS manufacturer authorize for an unregistered Part
103 paraglider?**

Until that question has a written authoritative answer, BONK must keep external
TABS transmit enable false on a Part 103 build.

**Exit criterion:** written confirmation from the FAA and/or selected TABS
manufacturer that defines the lawful identity/configuration and operating path
for the intended Part 103 paraglider.

### B2 — purchasable lightweight TABS hardware

The best SWaP fit found so far is not yet generally available.

**Exit criterion:** identify a currently purchasable unit with documented TABS
or suitable approved voluntary-equipage status, published interface documents,
and acceptable mass/power.

### B3 — pressure altitude

1090/TABS systems may require pressure altitude independent of BONK's GNSS
altitude. The selected hardware's requirements must be followed exactly.

**Exit criterion:** select an approved/internal encoder path and expose only its
valid/fault state to BONK.

### B4 — GNSS integrity / position source

A normal hobby GPS fix must not automatically be treated as a compliant ADS-B
position source. Some devices use an approved internal or external GNSS source;
others may permit a host interface under defined conditions.

**Exit criterion:** document the exact approved position-source architecture for
the selected transmitter.

### B5 — antenna and human-worn installation

A 1090 MHz aviation transmitter needs a real antenna installation with adequate
separation, feedline, orientation, body-shadowing analysis, and RF exposure
review. A paraglider harness is not a conventional metal airframe.

**Exit criterion:** antenna location and cable design survive bench/range tests
and the device manufacturer's installation requirements.

### B6 — power system

Target architecture should use a dedicated or independently protected supply so
a TABS fault cannot brown out FLARM/BONK.

**Exit criterion:** measured current transients, minimum flight endurance,
independent fuse/current limiting, brownout behavior, and low-battery policy are
validated on hardware.

## Firmware architecture

`include/bonk/tabs_interface.hpp` defines a device-neutral transport contract.
A future device driver should live behind that interface, e.g.:

```text
BONK GNSS/status ----> TabsTransport ----> supported external TABS device
                              ^
                              |
                         health/faults
```

The transport is deliberately asymmetric: BONK may request transmit enable, but
it never owns squitter timing or packet generation.

## Recommended next implementation

1. Resolve B1 (identity/Part 103 operating path) with the chosen manufacturer
   and FAA guidance.
2. Obtain one production transmitter for bench integration; TT23G is a viable
   heavy proof-of-concept if no lightweight TABS device is available.
3. Implement a driver only from the selected manufacturer's current interface
   control document.
4. Add simulated transport tests for stale GNSS, identity invalid, encoder
   failure, link loss, brownout, and transmit-disable behavior.
5. Only after those pass, wire the hardware on a bench with a dummy load or
   manufacturer-approved test setup before any radiated field testing.

## Success condition

BONK is successful when a paraglider can carry a lightweight external
conspicuity transmitter whose identity/configuration is lawful and whose RF is
visible to the intended airborne collision-avoidance/ADS-B-In ecosystem, while
FLARM and Meshtastic continue to operate independently.
