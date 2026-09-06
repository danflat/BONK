#include "bonk/radio_arbiter.hpp"

#include <algorithm>
#include <limits>

namespace bonk {

namespace {

RequestResult denied(DenialReason reason) noexcept {
    RequestResult result{};
    result.denial = reason;
    return result;
}

} // namespace

RadioArbiter::RadioArbiter(RadioArbiterConfig config) : config_(config) {}

bool RadioArbiter::scheduleFlarmWindow(FlarmWindow window) noexcept {
    if (!window.valid()) {
        return false;
    }
    flarm_window_ = window;
    return true;
}

void RadioArbiter::clearFlarmWindow() noexcept {
    flarm_window_ = {};
}

RequestResult RadioArbiter::requestFlarm(
    Micros now_us,
    RadioOperation operation,
    Micros operation_budget_us) noexcept {
    if (operation_budget_us == 0) {
        return denied(DenialReason::InvalidRequest);
    }

    const TickResult housekeeping = tick(now_us);
    bool mesh_preempted =
        housekeeping.revoked.valid() &&
        housekeeping.revoked.client == RadioClient::Meshtastic;

    if (active_.valid() && active_.client == RadioClient::Meshtastic) {
        active_ = {};
        mesh_preempted = true;
    } else if (active_.valid()) {
        active_ = {};
    }

    const Micros not_before = saturatingAdd(now_us, config_.radio_handoff_us);
    const Micros expires = saturatingAdd(not_before, operation_budget_us);
    if (expires <= not_before) {
        return denied(DenialReason::InvalidRequest);
    }

    active_ = {
        RadioClient::Flarm,
        operation,
        now_us,
        not_before,
        expires,
        nextToken(),
    };

    RequestResult result{};
    result.lease = active_;
    result.mesh_preempted = mesh_preempted;
    return result;
}

RequestResult RadioArbiter::requestMeshtastic(
    Micros now_us,
    RadioOperation operation,
    Micros operation_budget_us) noexcept {
    if (operation_budget_us == 0) {
        return denied(DenialReason::InvalidRequest);
    }

    const TickResult housekeeping = tick(now_us);
    if (housekeeping.event == ArbiterEvent::ClockRollback) {
        return denied(DenialReason::ClockRollback);
    }
    if (safe_hold_) {
        return denied(DenialReason::SafeHold);
    }
    if (!mesh_enabled_) {
        return denied(DenialReason::MeshDisabled);
    }
    if (active_.valid()) {
        return denied(DenialReason::Busy);
    }

    if (!flarm_window_.valid()) {
        if (config_.require_flarm_window_for_mesh) {
            return denied(DenialReason::MissingFlarmWindow);
        }
    } else {
        if (isFlarmProtected(now_us)) {
            return denied(DenialReason::FlarmProtected);
        }
        if (now_us >= protectedEnd()) {
            return denied(DenialReason::MissingFlarmWindow);
        }
    }

    Micros safe_end = std::numeric_limits<Micros>::max();
    if (flarm_window_.valid() && now_us < protectedStart()) {
        safe_end = protectedStart();
    }

    if (operation == RadioOperation::Transmit &&
        operation_budget_us > config_.max_mesh_tx_airtime_us) {
        return denied(DenialReason::TxAirtimeTooLong);
    }

    const Micros not_before = saturatingAdd(now_us, config_.radio_handoff_us);
    if (not_before >= safe_end) {
        return denied(DenialReason::InsufficientGap);
    }

    Micros usable_budget = operation_budget_us;
    if (operation == RadioOperation::Receive) {
        usable_budget = std::min(usable_budget, config_.max_mesh_rx_slice_us);
        usable_budget = std::min(usable_budget, safe_end - not_before);
        if (usable_budget < config_.minimum_mesh_slice_us) {
            return denied(DenialReason::InsufficientGap);
        }
    } else if (operation_budget_us > safe_end - not_before) {
        return denied(DenialReason::InsufficientGap);
    }

    const Micros expires = saturatingAdd(not_before, usable_budget);
    if (expires <= not_before || expires > safe_end) {
        return denied(DenialReason::InsufficientGap);
    }

    active_ = {
        RadioClient::Meshtastic,
        operation,
        now_us,
        not_before,
        expires,
        nextToken(),
    };

    RequestResult result{};
    result.lease = active_;
    return result;
}

TickResult RadioArbiter::tick(Micros now_us) noexcept {
    TickResult result{};

    if (clock_initialized_ && now_us < last_now_us_) {
        result.event = ArbiterEvent::ClockRollback;
        result.revoked = active_;
        active_ = {};
        safe_hold_ = true;
        last_now_us_ = now_us;
        result.flarm_protected = true;
        result.safe_hold = true;
        return result;
    }

    clock_initialized_ = true;
    last_now_us_ = now_us;

    if (active_.valid() &&
        active_.client == RadioClient::Meshtastic &&
        isFlarmProtected(now_us)) {
        result.event = ArbiterEvent::MeshPreempted;
        result.revoked = active_;
        active_ = {};
    } else if (active_.valid() && now_us >= active_.expires_at_us) {
        result.revoked = active_;
        result.event = active_.client == RadioClient::Flarm
                           ? ArbiterEvent::FlarmLeaseExpired
                           : ArbiterEvent::MeshLeaseExpired;
        active_ = {};
    }

    result.flarm_protected = safe_hold_ || isFlarmProtected(now_us);
    result.safe_hold = safe_hold_;
    return result;
}

bool RadioArbiter::release(std::uint32_t token, Micros now_us) noexcept {
    static_cast<void>(tick(now_us));
    if (!active_.valid() || token == 0 || token != active_.token) {
        return false;
    }
    active_ = {};
    return true;
}

void RadioArbiter::setMeshEnabled(bool enabled) noexcept {
    mesh_enabled_ = enabled;
    if (!mesh_enabled_ && active_.client == RadioClient::Meshtastic) {
        active_ = {};
    }
}

bool RadioArbiter::meshEnabled() const noexcept {
    return mesh_enabled_;
}

void RadioArbiter::enterSafeHold() noexcept {
    safe_hold_ = true;
    if (active_.client == RadioClient::Meshtastic) {
        active_ = {};
    }
}

void RadioArbiter::clearSafeHold(Micros now_us) noexcept {
    safe_hold_ = false;
    clock_initialized_ = true;
    last_now_us_ = now_us;
}

bool RadioArbiter::inSafeHold() const noexcept {
    return safe_hold_;
}

bool RadioArbiter::isFlarmProtected(Micros now_us) const noexcept {
    if (!flarm_window_.valid()) {
        return false;
    }
    return now_us >= protectedStart() && now_us < protectedEnd();
}

Micros RadioArbiter::meshBudgetBeforeFlarm(Micros now_us) const noexcept {
    if (safe_hold_ || !mesh_enabled_) {
        return 0;
    }
    if (!flarm_window_.valid()) {
        return config_.require_flarm_window_for_mesh
                   ? 0
                   : std::numeric_limits<Micros>::max();
    }
    if (isFlarmProtected(now_us) || now_us >= protectedEnd()) {
        return 0;
    }
    if (now_us >= protectedStart()) {
        return 0;
    }

    const Micros gap = protectedStart() - now_us;
    if (gap <= config_.radio_handoff_us) {
        return 0;
    }
    return gap - config_.radio_handoff_us;
}

const RadioLease &RadioArbiter::activeLease() const noexcept {
    return active_;
}

const FlarmWindow &RadioArbiter::flarmWindow() const noexcept {
    return flarm_window_;
}

const RadioArbiterConfig &RadioArbiter::config() const noexcept {
    return config_;
}

Micros RadioArbiter::saturatingAdd(Micros lhs, Micros rhs) noexcept {
    const Micros maximum = std::numeric_limits<Micros>::max();
    return rhs > maximum - lhs ? maximum : lhs + rhs;
}

Micros RadioArbiter::saturatingSub(Micros lhs, Micros rhs) noexcept {
    return rhs > lhs ? 0 : lhs - rhs;
}

Micros RadioArbiter::protectedStart() const noexcept {
    return saturatingSub(
        flarm_window_.start_us,
        saturatingAdd(config_.flarm_pre_guard_us, config_.radio_handoff_us));
}

Micros RadioArbiter::protectedEnd() const noexcept {
    return saturatingAdd(
        flarm_window_.end_us,
        saturatingAdd(config_.flarm_post_guard_us, config_.radio_handoff_us));
}

std::uint32_t RadioArbiter::nextToken() noexcept {
    std::uint32_t token = next_token_++;
    if (token == 0) {
        token = next_token_++;
    }
    return token;
}

} // namespace bonk
