#pragma once

#include "bonk/lora_airtime.hpp"
#include "bonk/radio_arbiter.hpp"

#include <cstddef>
#include <cstdint>

namespace bonk {

struct MeshTxPlan {
    std::size_t payload_bytes = 0;
    LoRaModem modem{};
    std::uint8_t packet_count = 1;
    Micros driver_margin_us = 5'000;
    Micros inter_packet_gap_us = 0;
};

class MeshtasticGate {
  public:
    explicit MeshtasticGate(RadioArbiter &arbiter) noexcept;

    [[nodiscard]] RequestResult requestReceiveSlice(
        Micros now_us,
        Micros requested_dwell_us) noexcept;

    [[nodiscard]] RequestResult requestTransmit(
        Micros now_us,
        const MeshTxPlan &plan) noexcept;

    [[nodiscard]] bool complete(
        const RadioLease &lease,
        Micros now_us) noexcept;

    [[nodiscard]] static Micros plannedAirtimeUs(
        const MeshTxPlan &plan) noexcept;

  private:
    RadioArbiter &arbiter_;
};

} // namespace bonk
