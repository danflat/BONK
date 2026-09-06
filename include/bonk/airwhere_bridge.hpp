#pragma once

#include <cstddef>
#include <cstdint>

namespace bonk {

struct HeltecV3Pins {
    static constexpr int gps_rx = 4;
    static constexpr int gps_tx = 5;
    static constexpr int flarm_rx = 6;
    static constexpr int flarm_tx = 7;
    static constexpr int led = 2;

    static constexpr int oled_sda = 17;
    static constexpr int oled_scl = 18;
    static constexpr int oled_reset = 21;
    static constexpr int vext = 36;

    static constexpr int battery_adc = 1;
    static constexpr int adc_control = 37;
    static constexpr int program_button = 0;

    static constexpr int lora_cs = 8;
    static constexpr int lora_sck = 9;
    static constexpr int lora_mosi = 10;
    static constexpr int lora_miso = 11;
    static constexpr int lora_reset = 12;
    static constexpr int lora_busy = 13;
    static constexpr int lora_dio1 = 14;
};

inline constexpr std::uint32_t kGpsBaud = 9'600;
inline constexpr std::uint32_t kFlarmBaud = 57'600;
inline constexpr std::uint8_t kOledAddress = 0x3C;
inline constexpr std::uint32_t kBleAdvertisingIntervalMs = 200;
inline constexpr std::uint32_t kUiRefreshIntervalMs = 3'000;
inline constexpr std::uint32_t kShutdownHoldMs = 2'000;

[[nodiscard]] bool parsePflauTargetCount(
    const std::uint8_t *data,
    std::size_t size,
    std::uint16_t &target_count) noexcept;

[[nodiscard]] std::uint8_t batteryPercentFromAdc(
    std::uint32_t raw,
    std::uint32_t full_scale,
    double reference_voltage = 3.3,
    double divider_multiplier = 4.9,
    double empty_voltage = 3.3,
    double full_voltage = 4.2) noexcept;

} // namespace bonk
