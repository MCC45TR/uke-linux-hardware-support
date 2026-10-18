// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace uke::battery {
using Number = std::optional<double>;
struct Estimate {
    Number seconds;
    std::string reason = "no_qualified_measurement";
    std::string confidence = "unknown";
};
struct Qualification {
    // These flags require recorded pack/firmware evidence, never a UI preference.
    bool current = false, energy = false, learned_capacity = false;
    bool capacity_basis = false, target_effective = false;
    std::optional<double> reserve_uwh;
    std::optional<double> efc_reference_uah;
    std::string profile = "uke-unqualified";
};
struct Observation {
    uint64_t boottime_ns = 0, monotonic_ns = 0;
    std::string battery_id, boot_id, epoch, conditions;
    Number current_ua, voltage_uv, energy_uwh, learned_full_uah, design_full_uah;
    Number soc, effective_target, vendor_soh, hardware_cycles;
    bool valid = false, hold = false, fault = false;
};
inline bool positive(Number n) { return n && std::isfinite(*n) && *n > 0; }
inline Number retention(const Observation &s, const Qualification &q) {
    if (!s.valid || !q.learned_capacity || !q.capacity_basis ||
        !positive(s.learned_full_uah) || !positive(s.design_full_uah)) return {};
    const double result = 100 * *s.learned_full_uah / *s.design_full_uah;
    return std::isfinite(result) ? Number(result) : Number{};
}
inline Estimate duration(double seconds, const char *method) {
    if (!std::isfinite(seconds) || seconds < 0 ||
        seconds >= static_cast<double>(std::numeric_limits<int64_t>::max()))
        return {{}, "estimate_out_of_range", "unknown"};
    return {seconds, method, "low"};
}
class Forecast {
    std::vector<Observation> window_;
    std::array<Number, 100> charge_curve_{};
    std::string conditions_;
public:
    void reset() { window_.clear(); charge_curve_.fill({}); conditions_.clear(); }
    void observe(const Observation &s, const Qualification &q) {
        if (!s.valid || !q.current || !s.current_ua || !std::isfinite(*s.current_ua) ||
            s.fault || s.hold) { reset(); return; }
        if (!window_.empty()) {
            const auto &last = window_.back();
            const auto dt = s.boottime_ns > last.boottime_ns ? s.boottime_ns - last.boottime_ns : 0;
            const auto dm = s.monotonic_ns > last.monotonic_ns ? s.monotonic_ns - last.monotonic_ns : 0;
            if (s.epoch != last.epoch || s.boot_id != last.boot_id ||
                s.battery_id != last.battery_id) reset();
            else if (dt == 0 || dt > 60000000000ULL ||
                dm == 0 || std::fabs(static_cast<double>(dt) - dm) > 1000000000.0 ||
                (*last.current_ua * *s.current_ua < 0) ||
                std::fabs(*s.current_ua - *last.current_ua) > std::max(1.0, std::fabs(*last.current_ua)) * 0.5)
                window_.clear();
        }
        // Curve keys must include qualified profile, contract, thermal and load bands.
        if (s.conditions.empty() || s.conditions != conditions_) {
            charge_curve_.fill({}); conditions_ = s.conditions;
        }
        window_.push_back(s);
        if (window_.size() > 12) window_.erase(window_.begin());
        if (!conditions_.empty() && s.soc && *s.soc >= 0 && *s.soc < 100 && *s.current_ua > 0) {
            auto &rate = charge_curve_[static_cast<size_t>(*s.soc)];
            rate = rate ? 0.75 * *rate + 0.25 * *s.current_ua : *s.current_ua;
        }
    }
    Estimate empty(const Observation &s, const Qualification &q) const {
        if (!s.valid || !q.current || !q.energy || !q.reserve_uwh ||
            !std::isfinite(*q.reserve_uwh) || *q.reserve_uwh < 0 || !positive(s.energy_uwh)) return {};
        if (window_.size() < 3 || window_.back().boottime_ns - window_.front().boottime_ns < 30000000000ULL)
            return {{}, "converging", "unknown"};
        double power = 0;
        for (const auto &v : window_) {
            if (!positive(v.voltage_uv) || !v.current_ua || *v.current_ua >= 0)
                return {{}, "not_net_discharging", "unknown"};
            power += -*v.voltage_uv * *v.current_ua / 1000000.0;
        }
        power /= window_.size();
        if (!std::isfinite(power) || power <= 0) return {{}, "rate_unavailable", "unknown"};
        return duration(3600 * std::max(0.0, *s.energy_uwh - *q.reserve_uwh) / power,
                        "net_battery_power_with_reserve");
    }
    Estimate target(const Observation &s, const Qualification &q, double target_soc) const {
        if (!s.valid || !q.current || !q.learned_capacity || !q.capacity_basis ||
            !q.target_effective || !s.effective_target || target_soc != *s.effective_target ||
            !s.soc || !std::isfinite(*s.soc) || *s.soc < 0 || *s.soc > 100 ||
            !std::isfinite(target_soc) || target_soc < 0 || target_soc > 100 ||
            !positive(s.learned_full_uah)) return {};
        if (s.hold || s.fault) return {{}, "paused_or_fault", "unknown"};
        if (*s.soc >= target_soc) return {0, "target_reached", "low"};
        if (!s.current_ua || *s.current_ua <= 0) return {{}, "not_net_charging", "unknown"};
        if (window_.size() < 3) return {{}, "converging", "unknown"};
        double seconds = 0, soc = *s.soc;
        while (soc < target_soc) {
            const size_t band = static_cast<size_t>(soc);
            if (band >= charge_curve_.size() || !positive(charge_curve_[band]))
                return {{}, "charge_curve_incomplete", "unknown"};
            const double next = std::min(target_soc, std::floor(soc) + 1);
            seconds += 3600 * *s.learned_full_uah * (next - soc) / (100 * *charge_curve_[band]);
            soc = next;
        }
        return duration(seconds, "observed_soc_charge_curve");
    }
};
struct EfcState {
    uint64_t sequence = 0, last_boottime_ns = 0, last_monotonic_ns = 0;
    std::string battery_id, boot_id, epoch;
    double reference_uah = 0, discharge_uah = 0, covered_seconds = 0, uncovered_seconds = 0;
    Number last_current_ua;
};
class Efc {
    EfcState state_;
public:
    explicit Efc(EfcState s = {}) : state_(std::move(s)) {
        // A process restart leaves an unobserved gap even within the same boot.
        state_.last_current_ua.reset();
    }
    const EfcState &state() const { return state_; }
    void break_continuity() { state_.last_current_ua.reset(); }
    Number cycles() const {
        if (state_.reference_uah <= 0 || state_.covered_seconds <= 0) return {};
        return state_.discharge_uah / state_.reference_uah;
    }
    void observe(const Observation &s, const Qualification &q, double reference_uah) {
        if (!q.current || !q.capacity_basis || s.battery_id.empty() || s.boot_id.empty() ||
            !std::isfinite(reference_uah) || reference_uah <= 0) return;
        if (state_.battery_id == s.battery_id && state_.reference_uah != reference_uah) {
            state_.last_current_ua.reset(); return; // Reference changes need a new explicit period.
        }
        if (state_.battery_id != s.battery_id) {
            const uint64_t sequence = state_.sequence;
            state_ = {}; state_.sequence = sequence;
            state_.battery_id = s.battery_id; state_.reference_uah = reference_uah;
        }
        double dt = s.boottime_ns > state_.last_boottime_ns ? (s.boottime_ns - state_.last_boottime_ns) / 1e9 : 0;
        double dm = s.monotonic_ns > state_.last_monotonic_ns ? (s.monotonic_ns - state_.last_monotonic_ns) / 1e9 : 0;
        const bool continuous = s.valid && !s.fault && s.current_ua && std::isfinite(*s.current_ua) &&
            state_.last_current_ua && state_.boot_id == s.boot_id && state_.epoch == s.epoch &&
            dt > 0 && dt <= 60 && dm > 0 && std::fabs(dt - dm) <= 1;
        if (continuous) {
            const double a = -*state_.last_current_ua, b = -*s.current_ua;
            double average = 0;
            if (a >= 0 && b >= 0) average = (a + b) / 2;
            else if (a > 0 || b > 0) {
                const double discharge = std::max(a, b);
                average = discharge * (discharge / std::fabs(a - b)) / 2;
            }
            const double delta = average * dt / 3600;
            if (std::isfinite(delta) && std::isfinite(state_.discharge_uah + delta)) {
                state_.discharge_uah += delta; state_.covered_seconds += dt;
            } else state_.uncovered_seconds += dt;
        } else if (state_.last_boottime_ns && state_.boot_id == s.boot_id && dt > 0)
            state_.uncovered_seconds += dt;
        state_.last_current_ua = s.valid && !s.fault && s.current_ua && std::isfinite(*s.current_ua) ? s.current_ua : Number{};
        state_.last_boottime_ns = s.boottime_ns; state_.last_monotonic_ns = s.monotonic_ns;
        state_.boot_id = s.boot_id; state_.epoch = s.epoch;
        if (state_.sequence != std::numeric_limits<uint64_t>::max()) ++state_.sequence;
    }
};
// The whole accumulator and its cursor are one checksummed, atomic record.
void save_checkpoint(const std::string &directory, const EfcState &s);
std::optional<EfcState> load_checkpoint(const std::string &directory);
}
