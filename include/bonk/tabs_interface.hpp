#pragma once

#include <cstdint>

namespace bonk {

// BONK never generates 1090 MHz or UAT RF. This interface is only for
// exchanging ownship data, control state, and health with external,
// purpose-built electronic-conspicuity equipment.
enum class TabsState : std::uint8_t {
    Unavailable,
    Standby,
    Ready,
    Transmitting,
    Fault,
};

struct TabsOwnship {
    std::int32_t latitude_e7 = 0;
    std::int32_t longitude_e7 = 0;
    std::int32_t geometric_altitude_cm = 0;
    std::int32_t vertical_speed_cms = 0;
    std::uint32_t ground_speed_cms = 0;
    std::uint16_t track_cdeg = 0;
    std::uint64_t fix_time_us = 0;
    bool position_valid = false;
};

struct TabsStatus {
    TabsState state = TabsState::Unavailable;
    bool identity_valid = false;
    bool position_source_valid = false;
    bool pressure_altitude_valid = false;
    bool rf_enabled = false;
    std::uint32_t fault_flags = 0;
};

class TabsTransport {
  public:
    virtual ~TabsTransport() = default;

    // Bring up only the data/control transport. Implementations must not
    // silently enable RF as a side effect of begin().
    virtual bool begin() noexcept = 0;

    // Feed fresh ownship state when the selected external device/protocol
    // explicitly supports host-provided navigation data. Certified devices
    // may instead use their own approved GNSS source and ignore this call.
    virtual bool updateOwnship(const TabsOwnship &ownship) noexcept = 0;

    // RF enable remains an explicit operation so firmware can enforce a
    // jurisdiction/device-specific authorization gate before transmission.
    virtual bool setTransmitEnabled(bool enabled) noexcept = 0;

    [[nodiscard]] virtual TabsStatus status() const noexcept = 0;
};

} // namespace bonk
