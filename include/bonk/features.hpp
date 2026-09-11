#pragma once

#ifndef BONK_ENABLE_ADSB_OUT
#define BONK_ENABLE_ADSB_OUT 0
#endif

static_assert(
    BONK_ENABLE_ADSB_OUT == 0,
    "BONK intentionally has no software-enabled ADS-B Out transmitter. "
    "Use the documented external TABS/certified-equipment integration path.");

namespace bonk {

// Direct 1090ES/UAT encoding and RF transmission are intentionally outside
// BONK. External TSO-C199/certified traffic equipment may be integrated over a
// documented data/control transport without weakening this invariant.
inline constexpr bool kAdsbOutEnabled = false;
inline constexpr bool kExternalTabsTransportSupported = true;
inline constexpr bool kMeshtasticBackgroundMode = true;
inline constexpr bool kFlarmHasAbsoluteRadioPriority = true;

} // namespace bonk
