#include "bonk/ble_tx_mux.hpp"
#include "bonk/features.hpp"
#include "bonk/lora_airtime.hpp"
#include "bonk/meshtastic_gate.hpp"
#include "bonk/radio_arbiter.hpp"
#include "bonk/splash.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failures = 0;
int assertions = 0;

#define EXPECT_TRUE(expression)                                                  \
    do {                                                                         \
        ++assertions;                                                            \
        if (!(expression)) {                                                     \
            std::cerr << __FILE__ << ':' << __LINE__                             \
                      << ": expected true: " #expression << '\n';               \
            ++failures;                                                          \
        }                                                                        \
    } while (false)

#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))

#define EXPECT_EQ(actual, expected)                                              \
    do {                                                                         \
        ++assertions;                                                            \
        if (!((actual) == (expected))) {                                         \
            std::cerr << __FILE__ << ':' << __LINE__                             \
                      << ": values differ: " #actual " and " #expected << '\n'; \
            ++failures;                                                          \
        }                                                                        \
    } while (false)

bonk::RadioArbiterConfig testConfig() {
    bonk::RadioArbiterConfig config{};
    config.radio_handoff_us = 5'000;
    config.flarm_pre_guard_us = 20'000;
    config.flarm_post_guard_us = 10'000;
    config.max_mesh_rx_slice_us = 75'000;
    config.max_mesh_tx_airtime_us = 150'000;
    config.minimum_mesh_slice_us = 5'000;
    return config;
}

bonk::RadioArbiter scheduledArbiter() {
    bonk::RadioArbiter arbiter{testConfig()};
    EXPECT_TRUE(arbiter.scheduleFlarmWindow({1'000'000, 1'100'000}));
    return arbiter;
}

void testInvalidWindowAndMissingScheduleFailSafe() {
    bonk::RadioArbiter arbiter{testConfig()};
    EXPECT_FALSE(arbiter.scheduleFlarmWindow({100, 100}));

    const auto result = arbiter.requestMeshtastic(
        10'000, bonk::RadioOperation::Receive, 20'000);
    EXPECT_FALSE(result.granted());
    EXPECT_EQ(result.denial, bonk::DenialReason::MissingFlarmWindow);
}

void testMeshReceiveIsBounded() {
    auto arbiter = scheduledArbiter();
    const auto result = arbiter.requestMeshtastic(
        100'000, bonk::RadioOperation::Receive, 200'000);

    EXPECT_TRUE(result.granted());
    EXPECT_EQ(result.lease.client, bonk::RadioClient::Meshtastic);
    EXPECT_EQ(result.lease.not_before_us, bonk::Micros{105'000});
    EXPECT_EQ(result.lease.expires_at_us, bonk::Micros{180'000});
    EXPECT_TRUE(arbiter.release(result.lease.token, 150'000));
    EXPECT_FALSE(arbiter.release(result.lease.token, 150'001));
}

void testMeshTransmitMustFitCompletely() {
    auto arbiter = scheduledArbiter();

    const auto too_long = arbiter.requestMeshtastic(
        100'000, bonk::RadioOperation::Transmit, 150'001);
    EXPECT_EQ(too_long.denial, bonk::DenialReason::TxAirtimeTooLong);

    const auto misses_guard = arbiter.requestMeshtastic(
        900'000, bonk::RadioOperation::Transmit, 70'001);
    EXPECT_EQ(misses_guard.denial, bonk::DenialReason::InsufficientGap);

    const auto exact_fit = arbiter.requestMeshtastic(
        900'000, bonk::RadioOperation::Transmit, 70'000);
    EXPECT_TRUE(exact_fit.granted());
    EXPECT_EQ(exact_fit.lease.expires_at_us, bonk::Micros{975'000});
}

void testProtectedWindowBlocksMesh() {
    auto arbiter = scheduledArbiter();
    EXPECT_TRUE(arbiter.isFlarmProtected(975'000));
    EXPECT_TRUE(arbiter.isFlarmProtected(1'114'999));
    EXPECT_FALSE(arbiter.isFlarmProtected(1'115'000));

    const auto during = arbiter.requestMeshtastic(
        975'000, bonk::RadioOperation::Receive, 10'000);
    EXPECT_EQ(during.denial, bonk::DenialReason::FlarmProtected);

    const auto stale_schedule = arbiter.requestMeshtastic(
        1'115'000, bonk::RadioOperation::Receive, 10'000);
    EXPECT_EQ(stale_schedule.denial, bonk::DenialReason::MissingFlarmWindow);
}

void testFlarmPreemptsMeshImmediately() {
    auto arbiter = scheduledArbiter();
    const auto mesh = arbiter.requestMeshtastic(
        100'000, bonk::RadioOperation::Receive, 50'000);
    EXPECT_TRUE(mesh.granted());

    const auto flarm = arbiter.requestFlarm(
        110'000, bonk::RadioOperation::Transmit, 30'000);
    EXPECT_TRUE(flarm.granted());
    EXPECT_TRUE(flarm.mesh_preempted);
    EXPECT_EQ(arbiter.activeLease().client, bonk::RadioClient::Flarm);
    EXPECT_FALSE(arbiter.release(mesh.lease.token, 111'000));
}

void testFlarmRejectsClockOverflow() {
    bonk::RadioArbiter arbiter{testConfig()};
    const auto result = arbiter.requestFlarm(
        std::numeric_limits<bonk::Micros>::max() - 1,
        bonk::RadioOperation::Transmit,
        30'000);
    EXPECT_FALSE(result.granted());
    EXPECT_EQ(result.denial, bonk::DenialReason::InvalidRequest);
}

void testTickRevokesMeshAtGuardBoundary() {
    auto arbiter = scheduledArbiter();
    const auto mesh = arbiter.requestMeshtastic(
        900'000, bonk::RadioOperation::Receive, 100'000);
    EXPECT_TRUE(mesh.granted());
    EXPECT_EQ(mesh.lease.expires_at_us, bonk::Micros{975'000});

    const auto tick = arbiter.tick(975'000);
    EXPECT_EQ(tick.event, bonk::ArbiterEvent::MeshPreempted);
    EXPECT_EQ(tick.revoked.token, mesh.lease.token);
    EXPECT_TRUE(tick.flarm_protected);
    EXPECT_FALSE(arbiter.activeLease().valid());
}

void testClockRollbackEntersSafeHold() {
    auto arbiter = scheduledArbiter();
    EXPECT_EQ(arbiter.tick(500'000).event, bonk::ArbiterEvent::None);
    const auto rollback = arbiter.tick(499'999);
    EXPECT_EQ(rollback.event, bonk::ArbiterEvent::ClockRollback);
    EXPECT_TRUE(arbiter.inSafeHold());

    const auto mesh = arbiter.requestMeshtastic(
        499'999, bonk::RadioOperation::Receive, 20'000);
    EXPECT_EQ(mesh.denial, bonk::DenialReason::SafeHold);

    const auto flarm = arbiter.requestFlarm(
        499'999, bonk::RadioOperation::Receive, 20'000);
    EXPECT_TRUE(flarm.granted());

    arbiter.clearSafeHold(500'000);
    EXPECT_FALSE(arbiter.inSafeHold());
}

void testMeshEnableSwitchRevokesMesh() {
    auto arbiter = scheduledArbiter();
    const auto mesh = arbiter.requestMeshtastic(
        100'000, bonk::RadioOperation::Receive, 20'000);
    EXPECT_TRUE(mesh.granted());

    arbiter.setMeshEnabled(false);
    EXPECT_FALSE(arbiter.activeLease().valid());
    const auto disabled = arbiter.requestMeshtastic(
        101'000, bonk::RadioOperation::Receive, 20'000);
    EXPECT_EQ(disabled.denial, bonk::DenialReason::MeshDisabled);
}

void testLoRaAirtimeAndMeshtasticGate() {
    const bonk::LoRaModem modem{};
    EXPECT_TRUE(modem.valid());
    EXPECT_EQ(bonk::loraAirtimeUs(16, modem), bonk::Micros{51'456});
    EXPECT_EQ(bonk::loraAirtimeUs(256, modem), bonk::Micros{0});

    bonk::LoRaModem invalid = modem;
    invalid.spreading_factor = 4;
    EXPECT_EQ(bonk::loraAirtimeUs(16, invalid), bonk::Micros{0});

    auto arbiter = scheduledArbiter();
    bonk::MeshtasticGate gate{arbiter};
    bonk::MeshTxPlan plan{};
    plan.payload_bytes = 16;
    EXPECT_EQ(
        bonk::MeshtasticGate::plannedAirtimeUs(plan),
        bonk::Micros{56'456});

    const auto tx = gate.requestTransmit(100'000, plan);
    EXPECT_TRUE(tx.granted());
    EXPECT_EQ(tx.lease.operation, bonk::RadioOperation::Transmit);
    EXPECT_TRUE(gate.complete(tx.lease, 160'000));

    plan.packet_count = 0;
    EXPECT_EQ(
        gate.requestTransmit(170'000, plan).denial,
        bonk::DenialReason::InvalidRequest);
}

void testBleMuxPreservesFlarmPriority() {
    bonk::BleTxMux mux{};
    EXPECT_EQ(bonk::kFlarmBleServiceUuid, std::uint16_t{0xFFE0});
    EXPECT_EQ(bonk::kFlarmBleCharacteristicUuid, std::uint16_t{0xFFE1});
    EXPECT_TRUE(mux.enqueueText(bonk::BleChannel::Meshtastic, "mesh"));
    EXPECT_TRUE(mux.enqueueText(bonk::BleChannel::FlarmTelemetry, "flarm"));

    bonk::BleFrame frame{};
    EXPECT_TRUE(mux.pop(frame));
    EXPECT_EQ(frame.channel, bonk::BleChannel::FlarmTelemetry);
    EXPECT_EQ(frame.size, std::size_t{5});
    EXPECT_EQ(frame.payload[0], static_cast<std::uint8_t>('f'));
    EXPECT_TRUE(mux.pop(frame));
    EXPECT_EQ(frame.channel, bonk::BleChannel::Meshtastic);
}

void testBleOverflowPolicies() {
    bonk::BleTxMux flarm_mux{};
    for (std::size_t index = 0; index <= bonk::BleTxMux::kQueueDepth; ++index) {
        const auto value = static_cast<std::uint8_t>(index);
        EXPECT_TRUE(flarm_mux.enqueue(
            bonk::BleChannel::FlarmTelemetry, &value, 1));
    }
    EXPECT_EQ(flarm_mux.stats().dropped_flarm, std::uint32_t{1});
    bonk::BleFrame frame{};
    EXPECT_TRUE(flarm_mux.pop(frame));
    EXPECT_EQ(frame.payload[0], std::uint8_t{1});

    bonk::BleTxMux mesh_mux{};
    const std::uint8_t value = 42;
    for (std::size_t index = 0; index < bonk::BleTxMux::kQueueDepth; ++index) {
        EXPECT_TRUE(mesh_mux.enqueue(bonk::BleChannel::Meshtastic, &value, 1));
    }
    EXPECT_FALSE(mesh_mux.enqueue(bonk::BleChannel::Meshtastic, &value, 1));
    EXPECT_EQ(mesh_mux.stats().dropped_mesh, std::uint32_t{1});
}

void testSplashAndFeaturePolicy() {
    const auto ready = bonk::makeSplashFrame(bonk::BootStage::Ready);
    EXPECT_EQ(ready.product, std::string_view{"BONK"});
    EXPECT_EQ(ready.tagline, std::string_view{"FLARM FIRST / MESH SECOND"});
    EXPECT_EQ(ready.status, std::string_view{"READY TO BONK"});
    EXPECT_EQ(ready.progress_percent, std::uint8_t{100});

    const auto fault = bonk::makeSplashFrame(bonk::BootStage::Fault);
    EXPECT_EQ(fault.status, std::string_view{"FLARM SAFE HOLD"});
    EXPECT_FALSE(bonk::kAdsbOutEnabled);
    EXPECT_TRUE(bonk::kMeshtasticBackgroundMode);
    EXPECT_TRUE(bonk::kFlarmHasAbsoluteRadioPriority);
}

} // namespace

int main() {
    testInvalidWindowAndMissingScheduleFailSafe();
    testMeshReceiveIsBounded();
    testMeshTransmitMustFitCompletely();
    testProtectedWindowBlocksMesh();
    testFlarmPreemptsMeshImmediately();
    testFlarmRejectsClockOverflow();
    testTickRevokesMeshAtGuardBoundary();
    testClockRollbackEntersSafeHold();
    testMeshEnableSwitchRevokesMesh();
    testLoRaAirtimeAndMeshtasticGate();
    testBleMuxPreservesFlarmPriority();
    testBleOverflowPolicies();
    testSplashAndFeaturePolicy();

    if (failures != 0) {
        std::cerr << failures << " of " << assertions << " assertions failed\n";
        return 1;
    }
    std::cout << "BONK: " << assertions << " assertions passed\n";
    return 0;
}
