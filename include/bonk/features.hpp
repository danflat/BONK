#pragma once

#ifndef BONK_ENABLE_ADSB_OUT
#define BONK_ENABLE_ADSB_OUT 0
#endif

static_assert(
    BONK_ENABLE_ADSB_OUT == 0,
    "BONK vNext intentionally has no software-enabled ADS-B Out transmitter. "
    "Use the documented certified-equipment integration path.");

namespace bonk {

inline constexpr bool kAdsbOutEnabled = false;
inline constexpr bool kMeshtasticBackgroundMode = true;
inline constexpr bool kFlarmHasAbsoluteRadioPriority = true;

} // namespace bonk
