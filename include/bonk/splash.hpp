#pragma once

#include <cstdint>
#include <string_view>

namespace bonk {

enum class BootStage : std::uint8_t {
    Starting,
    RadioSelfTest,
    FlarmReady,
    BleReady,
    MeshStandby,
    Ready,
    Fault,
};

struct SplashFrame {
    std::string_view product;
    std::string_view tagline;
    std::string_view status;
    std::uint8_t progress_percent = 0;
};

[[nodiscard]] SplashFrame makeSplashFrame(BootStage stage) noexcept;

} // namespace bonk
