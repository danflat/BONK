#pragma once

#include "bonk/radio_arbiter.hpp"

#include <cstddef>
#include <cstdint>

namespace bonk {

struct LoRaModem {
    std::uint8_t spreading_factor = 7;
    std::uint32_t bandwidth_hz = 125'000;
    std::uint8_t coding_rate_denominator = 5;
    std::uint16_t preamble_symbols = 8;
    bool explicit_header = true;
    bool crc_enabled = true;
    bool auto_low_data_rate_optimize = true;
    bool low_data_rate_optimize = false;

    [[nodiscard]] bool valid() const noexcept;
};

[[nodiscard]] Micros loraAirtimeUs(
    std::size_t payload_bytes,
    const LoRaModem &modem) noexcept;

} // namespace bonk
