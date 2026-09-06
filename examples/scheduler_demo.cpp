#include "bonk/meshtastic_gate.hpp"
#include "bonk/radio_arbiter.hpp"

#include <iostream>

namespace {

const char *clientName(bonk::RadioClient client) {
    switch (client) {
    case bonk::RadioClient::Flarm:
        return "FLARM";
    case bonk::RadioClient::Meshtastic:
        return "Meshtastic";
    case bonk::RadioClient::None:
        return "none";
    }
    return "unknown";
}

void printLease(const bonk::RequestResult &result) {
    if (!result.granted()) {
        std::cout << "denied (reason=" << static_cast<int>(result.denial)
                  << ")\n";
        return;
    }
    std::cout << clientName(result.lease.client)
              << " lease: ready=" << result.lease.not_before_us
              << " us, expires=" << result.lease.expires_at_us << " us\n";
}

} // namespace

int main() {
    bonk::RadioArbiter arbiter{};
    if (!arbiter.scheduleFlarmWindow({1'000'000, 1'120'000})) {
        return 1;
    }
    bonk::MeshtasticGate mesh{arbiter};

    std::cout << "next FLARM window: [1000000, 1120000) us\n";
    std::cout << "mesh RX at t=100000: ";
    const auto rx = mesh.requestReceiveSlice(100'000, 200'000);
    printLease(rx);
    if (rx.granted()) {
        static_cast<void>(mesh.complete(rx.lease, 170'000));
    }

    bonk::MeshTxPlan packet{};
    packet.payload_bytes = 16;
    std::cout << "mesh TX at t=900000: ";
    const auto tx = mesh.requestTransmit(900'000, packet);
    printLease(tx);
    if (tx.granted()) {
        static_cast<void>(mesh.complete(tx.lease, tx.lease.expires_at_us));
    }

    std::cout << "mesh RX in FLARM guard: ";
    printLease(mesh.requestReceiveSlice(967'000, 20'000));

    std::cout << "urgent FLARM at t=970000: ";
    const auto flarm = arbiter.requestFlarm(
        970'000, bonk::RadioOperation::Transmit, 100'000);
    printLease(flarm);

    return flarm.granted() ? 0 : 1;
}
