#include "bonk/meshtastic_gate.hpp"

#include <limits>

namespace bonk {

namespace {

Micros saturatingAdd(Micros lhs, Micros rhs) noexcept {
    const Micros maximum = std::numeric_limits<Micros>::max();
    return rhs > maximum - lhs ? maximum : lhs + rhs;
}

Micros saturatingMultiply(Micros lhs, Micros rhs) noexcept {
    if (lhs == 0 || rhs == 0) {
        return 0;
    }
    const Micros maximum = std::numeric_limits<Micros>::max();
    return lhs > maximum / rhs ? maximum : lhs * rhs;
}

RequestResult invalidRequest() noexcept {
    RequestResult result{};
    result.denial = DenialReason::InvalidRequest;
    return result;
}

} // namespace

MeshtasticGate::MeshtasticGate(RadioArbiter &arbiter) noexcept
    : arbiter_(arbiter) {}

RequestResult MeshtasticGate::requestReceiveSlice(
    Micros now_us,
    Micros requested_dwell_us) noexcept {
    return arbiter_.requestMeshtastic(
        now_us,
        RadioOperation::Receive,
        requested_dwell_us);
}

RequestResult MeshtasticGate::requestTransmit(
    Micros now_us,
    const MeshTxPlan &plan) noexcept {
    const Micros airtime = plannedAirtimeUs(plan);
    if (airtime == 0) {
        return invalidRequest();
    }
    return arbiter_.requestMeshtastic(
        now_us,
        RadioOperation::Transmit,
        airtime);
}

bool MeshtasticGate::complete(
    const RadioLease &lease,
    Micros now_us) noexcept {
    return arbiter_.release(lease.token, now_us);
}

Micros MeshtasticGate::plannedAirtimeUs(
    const MeshTxPlan &plan) noexcept {
    if (plan.packet_count == 0) {
        return 0;
    }

    const Micros one_packet =
        loraAirtimeUs(plan.payload_bytes, plan.modem);
    if (one_packet == 0) {
        return 0;
    }

    const Micros packets =
        saturatingMultiply(one_packet, plan.packet_count);
    const Micros gaps =
        saturatingMultiply(
            plan.inter_packet_gap_us,
            static_cast<Micros>(plan.packet_count - 1));
    return saturatingAdd(
        saturatingAdd(packets, gaps),
        plan.driver_margin_us);
}

} // namespace bonk
