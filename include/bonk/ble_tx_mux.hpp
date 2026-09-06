#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bonk {

inline constexpr std::uint16_t kFlarmBleServiceUuid = 0xFFE0;
inline constexpr std::uint16_t kFlarmBleCharacteristicUuid = 0xFFE1;

enum class BleChannel : std::uint8_t {
    FlarmTelemetry,
    Meshtastic,
};

struct BleFrame {
    static constexpr std::size_t kMaxPayload = 244;

    BleChannel channel = BleChannel::FlarmTelemetry;
    std::array<std::uint8_t, kMaxPayload> payload{};
    std::size_t size = 0;
    std::uint32_t sequence = 0;
};

struct BleMuxStats {
    std::uint32_t queued_flarm = 0;
    std::uint32_t queued_mesh = 0;
    std::uint32_t dropped_flarm = 0;
    std::uint32_t dropped_mesh = 0;
};

class BleTxMux {
  public:
    static constexpr std::size_t kQueueDepth = 8;

    [[nodiscard]] bool enqueue(
        BleChannel channel,
        const std::uint8_t *data,
        std::size_t size) noexcept;

    [[nodiscard]] bool enqueueText(
        BleChannel channel,
        const char *text) noexcept;

    [[nodiscard]] bool pop(BleFrame &frame) noexcept;
    [[nodiscard]] std::size_t pending(BleChannel channel) const noexcept;
    [[nodiscard]] const BleMuxStats &stats() const noexcept;

  private:
    struct Queue {
        std::array<BleFrame, kQueueDepth> frames{};
        std::size_t head = 0;
        std::size_t tail = 0;
        std::size_t count = 0;
    };

    [[nodiscard]] static bool push(
        Queue &queue,
        BleChannel channel,
        const std::uint8_t *data,
        std::size_t size,
        std::uint32_t sequence) noexcept;

    [[nodiscard]] static bool pull(
        Queue &queue,
        BleFrame &frame) noexcept;

    Queue flarm_{};
    Queue mesh_{};
    BleMuxStats stats_{};
    std::uint32_t next_sequence_ = 1;
};

} // namespace bonk
