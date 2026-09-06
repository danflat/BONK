#pragma once

#include <cstdint>

namespace bonk {

using Micros = std::uint64_t;

enum class RadioClient : std::uint8_t {
    None,
    Flarm,
    Meshtastic,
};

enum class RadioOperation : std::uint8_t {
    Receive,
    Transmit,
};

enum class DenialReason : std::uint8_t {
    None,
    InvalidRequest,
    Busy,
    MeshDisabled,
    MissingFlarmWindow,
    FlarmProtected,
    InsufficientGap,
    TxAirtimeTooLong,
    SafeHold,
    ClockRollback,
};

enum class ArbiterEvent : std::uint8_t {
    None,
    FlarmLeaseExpired,
    MeshLeaseExpired,
    MeshPreempted,
    ClockRollback,
};

struct RadioArbiterConfig {
    Micros radio_handoff_us = 8'000;
    Micros flarm_pre_guard_us = 25'000;
    Micros flarm_post_guard_us = 10'000;
    Micros max_mesh_rx_slice_us = 75'000;
    Micros max_mesh_tx_airtime_us = 150'000;
    Micros minimum_mesh_slice_us = 5'000;
    bool require_flarm_window_for_mesh = true;
};

struct FlarmWindow {
    Micros start_us = 0;
    Micros end_us = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return end_us > start_us;
    }
};

struct RadioLease {
    RadioClient client = RadioClient::None;
    RadioOperation operation = RadioOperation::Receive;
    Micros granted_at_us = 0;
    Micros not_before_us = 0;
    Micros expires_at_us = 0;
    std::uint32_t token = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return client != RadioClient::None &&
               token != 0 &&
               expires_at_us > not_before_us;
    }
};

struct RequestResult {
    RadioLease lease{};
    DenialReason denial = DenialReason::None;
    bool mesh_preempted = false;

    [[nodiscard]] constexpr bool granted() const noexcept {
        return lease.valid() && denial == DenialReason::None;
    }
};

struct TickResult {
    ArbiterEvent event = ArbiterEvent::None;
    RadioLease revoked{};
    bool flarm_protected = false;
    bool safe_hold = false;
};

class RadioArbiter {
  public:
    explicit RadioArbiter(RadioArbiterConfig config = {});

    [[nodiscard]] bool scheduleFlarmWindow(FlarmWindow window) noexcept;
    void clearFlarmWindow() noexcept;

    [[nodiscard]] RequestResult requestFlarm(
        Micros now_us,
        RadioOperation operation,
        Micros operation_budget_us) noexcept;

    [[nodiscard]] RequestResult requestMeshtastic(
        Micros now_us,
        RadioOperation operation,
        Micros operation_budget_us) noexcept;

    [[nodiscard]] TickResult tick(Micros now_us) noexcept;
    [[nodiscard]] bool release(std::uint32_t token, Micros now_us) noexcept;

    void setMeshEnabled(bool enabled) noexcept;
    [[nodiscard]] bool meshEnabled() const noexcept;

    void enterSafeHold() noexcept;
    void clearSafeHold(Micros now_us) noexcept;
    [[nodiscard]] bool inSafeHold() const noexcept;

    [[nodiscard]] bool isFlarmProtected(Micros now_us) const noexcept;
    [[nodiscard]] Micros meshBudgetBeforeFlarm(Micros now_us) const noexcept;
    [[nodiscard]] const RadioLease &activeLease() const noexcept;
    [[nodiscard]] const FlarmWindow &flarmWindow() const noexcept;
    [[nodiscard]] const RadioArbiterConfig &config() const noexcept;

  private:
    [[nodiscard]] static Micros saturatingAdd(Micros lhs, Micros rhs) noexcept;
    [[nodiscard]] static Micros saturatingSub(Micros lhs, Micros rhs) noexcept;
    [[nodiscard]] Micros protectedStart() const noexcept;
    [[nodiscard]] Micros protectedEnd() const noexcept;
    [[nodiscard]] std::uint32_t nextToken() noexcept;

    RadioArbiterConfig config_{};
    FlarmWindow flarm_window_{};
    RadioLease active_{};
    Micros last_now_us_ = 0;
    std::uint32_t next_token_ = 1;
    bool clock_initialized_ = false;
    bool mesh_enabled_ = true;
    bool safe_hold_ = false;
};

} // namespace bonk
