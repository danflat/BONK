# ADS-B Out policy

ADS-B Out is a roadmap item, not an enabled BONK feature.

`include/bonk/features.hpp` pins `BONK_ENABLE_ADSB_OUT` to zero with a
compile-time assertion. This repository contains no ADS-B message encoder,
1090 MHz or UAT modulator, RF driver, or configuration path that can turn BONK
into an ADS-B Out transmitter.

## Why the boundary is strict

ADS-B Out broadcasts aircraft identity, position, altitude, and velocity for
surveillance. Installation, equipment performance, identifiers, and operating
rules depend on jurisdiction and airspace. In the United States, the FAA's
[ADS-B frequently asked questions](https://www.faa.gov/air_traffic/technology/equipadsb/resources/faq)
are the starting point; they do not make a home-built firmware transmitter an
approved installation.

## Permitted future shape

A future BONK integration may produce navigation/traffic data for an external,
compliant ADS-B Out installation through a documented data interface. That work
must include:

1. A named jurisdiction and intended aircraft/operation.
2. Review by appropriately qualified avionics and regulatory specialists.
3. An explicit approved-equipment boundary: BONK does not generate RF.
4. Interface validation, stale-data rejection, integrity/status propagation,
   and a loss-of-link safe state.
5. No shared-radio scheduling path with FLARM or Meshtastic.

Receive-only ADS-B traffic ingestion is a separate possible feature and must not
be described as ADS-B Out.
