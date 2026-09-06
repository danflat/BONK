#include "bonk/splash.hpp"

namespace bonk {

SplashFrame makeSplashFrame(BootStage stage) noexcept {
    constexpr std::string_view product = "BONK";
    constexpr std::string_view tagline = "FLARM FIRST / MESH SECOND";

    switch (stage) {
    case BootStage::Starting:
        return {product, tagline, "WAKING UP", 5};
    case BootStage::RadioSelfTest:
        return {product, tagline, "CHECKING RADIO", 25};
    case BootStage::FlarmReady:
        return {product, tagline, "FLARM READY", 55};
    case BootStage::BleReady:
        return {product, tagline, "BLE READY", 72};
    case BootStage::MeshStandby:
        return {product, tagline, "MESH STANDBY", 88};
    case BootStage::Ready:
        return {product, tagline, "READY TO BONK", 100};
    case BootStage::Fault:
        return {product, tagline, "FLARM SAFE HOLD", 100};
    }
    return {product, tagline, "FLARM SAFE HOLD", 100};
}

} // namespace bonk
