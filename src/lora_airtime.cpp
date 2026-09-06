#include "bonk/lora_airtime.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bonk {

bool LoRaModem::valid() const noexcept {
    return spreading_factor >= 5 &&
           spreading_factor <= 12 &&
           bandwidth_hz > 0 &&
           coding_rate_denominator >= 5 &&
           coding_rate_denominator <= 8 &&
           preamble_symbols > 0;
}

Micros loraAirtimeUs(
    std::size_t payload_bytes,
    const LoRaModem &modem) noexcept {
    if (!modem.valid() || payload_bytes > 255) {
        return 0;
    }

    const double symbol_seconds =
        static_cast<double>(std::uint32_t{1} << modem.spreading_factor) /
        static_cast<double>(modem.bandwidth_hz);
    const bool low_data_rate_optimize =
        modem.low_data_rate_optimize ||
        (modem.auto_low_data_rate_optimize && symbol_seconds >= 0.016);

    const int sf = modem.spreading_factor;
    const int de = low_data_rate_optimize ? 1 : 0;
    const int ih = modem.explicit_header ? 0 : 1;
    const int crc = modem.crc_enabled ? 1 : 0;
    const int numerator =
        static_cast<int>(8U * payload_bytes) -
        (4 * sf) +
        28 +
        (16 * crc) -
        (20 * ih);
    const int denominator = 4 * (sf - (2 * de));

    const double encoded_blocks =
        numerator > 0
            ? std::ceil(
                  static_cast<double>(numerator) /
                  static_cast<double>(denominator))
            : 0.0;
    const double payload_symbols =
        8.0 +
        std::max(
            encoded_blocks *
                static_cast<double>(modem.coding_rate_denominator),
            0.0);
    const double preamble_symbols =
        static_cast<double>(modem.preamble_symbols) + 4.25;
    const double total_us =
        (preamble_symbols + payload_symbols) * symbol_seconds * 1'000'000.0;

    if (!std::isfinite(total_us) ||
        total_us > static_cast<double>(std::numeric_limits<Micros>::max())) {
        return 0;
    }
    return static_cast<Micros>(std::ceil(total_us));
}

} // namespace bonk
