#include "bonk/airwhere_bridge.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace bonk {

bool parsePflauTargetCount(
    const std::uint8_t *data,
    std::size_t size,
    std::uint16_t &target_count) noexcept {
    if (data == nullptr || size == 0) {
        return false;
    }

    constexpr std::array<std::uint8_t, 7> prefix{
        '$', 'P', 'F', 'L', 'A', 'U', ','};
    const auto begin = data;
    const auto end = data + size;
    const auto match = std::search(begin, end, prefix.begin(), prefix.end());
    if (match == end) {
        return false;
    }

    const auto field = match + prefix.size();
    if (field == end || *field < '0' || *field > '9') {
        return false;
    }

    std::uint32_t value = 0;
    auto cursor = field;
    while (cursor != end && *cursor != ',' && *cursor != '\r' && *cursor != '\n') {
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
        value = (value * 10U) + static_cast<std::uint32_t>(*cursor - '0');
        if (value > std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        ++cursor;
    }

    target_count = static_cast<std::uint16_t>(value);
    return true;
}

std::uint8_t batteryPercentFromAdc(
    std::uint32_t raw,
    std::uint32_t full_scale,
    double reference_voltage,
    double divider_multiplier,
    double empty_voltage,
    double full_voltage) noexcept {
    if (full_scale == 0 ||
        reference_voltage <= 0.0 ||
        divider_multiplier <= 0.0 ||
        full_voltage <= empty_voltage) {
        return 0;
    }

    const double bounded_raw = static_cast<double>(std::min(raw, full_scale));
    const double voltage =
        (bounded_raw / static_cast<double>(full_scale)) *
        reference_voltage *
        divider_multiplier;
    const double percent =
        ((voltage - empty_voltage) / (full_voltage - empty_voltage)) * 100.0;
    if (percent <= 0.0) {
        return 0;
    }
    if (percent >= 100.0) {
        return 100;
    }
    return static_cast<std::uint8_t>(percent);
}

} // namespace bonk
