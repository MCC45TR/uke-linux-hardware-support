// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace uke::charge {
enum class Target { full, cap80, band80_90 };
enum class Rate { firmware, slow, automatic };
struct Request { Target target = Target::full; Rate rate = Rate::firmware; };
inline std::optional<Request> preset(const std::string& name) {
    if (name == "firmware") return Request{};
    if (name == "charge80") return Request{Target::cap80, Rate::firmware};
    if (name == "hold80-90") return Request{Target::band80_90, Rate::firmware};
    if (name == "slow") return Request{Target::full, Rate::slow};
    if (name == "automatic") return Request{Target::full, Rate::automatic};
    return {};
}
struct Controls {
    // Independent, reviewed control admission; model attestation is insufficient.
    bool qualified = false, end_threshold = false, start_threshold = false;
    bool battery_current = false, battery_voltage = false, autonomous_protection = false;
    std::string epoch;
};
struct Sample {
    bool coherent = false, measurements_qualified = false, sink = false, fault = true;
    bool suspended = false, net_discharging = false;
    std::string epoch;
    uint64_t boottime_ns = 0, receipt_ns = 0;
    std::optional<double> soc, temperature_deci_c, battery_uv, battery_ua;
    // Available battery-side power after qualified conversion loss and system load.
    // VBUS * USB current without those terms must not populate this field.
    std::optional<double> available_battery_uw;
};
struct ThermalBand {
    double min_deci_c = 0, max_deci_c = 0;
    uint32_t max_battery_ua = 0, max_battery_uv = 0;
};
struct Envelope {
    bool qualified = false;
    std::string epoch;
    uint32_t pack_ua = 0, path_ua = 0, firmware_ua = 0, slow_ua = 0;
    uint32_t max_battery_uv = 0, increase_step_ua = 0;
    uint64_t max_age_ns = 0, max_gap_ns = 0, increase_dwell_ns = 0;
    // Exact pack/firmware profile, ascending, non-overlapping half-open bands.
    // A gap or a zero current band requires firmware charging inhibition.
    std::vector<ThermalBand> bands;
};
struct Plan {
    bool eligible = false;
    std::string state = "blocked", reason = "control_backend_unqualified";
    std::optional<unsigned> start_percent, end_percent;
    std::optional<uint32_t> battery_current_ua, battery_voltage_uv;
    // Informational SOC demand only. Never maps a zero ICL to a physical stop.
    std::string soc_demand = "unknown";
};
inline bool finite(const std::optional<double>& x) { return x && std::isfinite(*x); }
inline bool valid(const Envelope& e) {
    if (!e.qualified || e.epoch.empty() || !e.pack_ua || !e.path_ua || !e.firmware_ua ||
        !e.slow_ua || !e.max_battery_uv || !e.increase_step_ua || !e.max_age_ns ||
        !e.max_gap_ns || !e.increase_dwell_ns || e.bands.empty()) return false;
    for (size_t i = 0; i < e.bands.size(); ++i) {
        const auto& b = e.bands[i];
        if (!std::isfinite(b.min_deci_c) || !std::isfinite(b.max_deci_c) ||
            b.min_deci_c >= b.max_deci_c || !b.max_battery_uv ||
            b.max_battery_uv > e.max_battery_uv ||
            (i && e.bands[i - 1].max_deci_c > b.min_deci_c)) return false;
    }
    return true;
}
// Pure recommendations. This class has no files, timers, firmware API or actuator.
class Planner {
    std::string epoch_;
    std::optional<bool> soc_charge_;
    std::optional<uint32_t> previous_ua_, previous_uv_;
    std::optional<Request> previous_request_;
    uint64_t last_ns_ = 0, stable_since_ns_ = 0;
    uint32_t candidate_ua_ = 0, candidate_uv_ = 0;
public:
    void reset() { *this = Planner{}; }
    Plan evaluate(Request request, const Sample& s, const Envelope& e, const Controls& c) {
        Plan p;
        if (s.epoch == epoch_ && last_ns_ && s.boottime_ns <= last_ns_) {
            previous_ua_.reset(); previous_uv_.reset(); soc_charge_.reset(); stable_since_ns_ = 0;
            p.reason = "sample_time_not_increasing"; return p;
        }
        const bool same_request = previous_request_ && previous_request_->target == request.target &&
            previous_request_->rate == request.rate;
        if (s.epoch != epoch_ || !same_request || !s.boottime_ns ||
            (last_ns_ && e.max_gap_ns && s.boottime_ns - last_ns_ > e.max_gap_ns)) reset();
        epoch_ = s.epoch; last_ns_ = s.boottime_ns; previous_request_ = request;
        const auto blocked = [&](const char* reason) {
            p = {}; p.reason = reason; previous_ua_.reset(); previous_uv_.reset();
            soc_charge_.reset(); stable_since_ns_ = 0; return p;
        };
        if (request.target == Target::full && request.rate == Rate::firmware) {
            p.state = "firmware_managed"; p.reason = "no_ap_control_requested";
            return p;
        }
        if (!c.qualified || !c.autonomous_protection || c.epoch.empty() || c.epoch != s.epoch)
            return blocked("control_backend_unqualified");
        if (!s.coherent || !s.measurements_qualified || s.suspended || s.fault || !s.sink ||
            !s.receipt_ns || s.boottime_ns < s.receipt_ns || !e.max_age_ns || !e.max_gap_ns ||
            s.boottime_ns - s.receipt_ns > e.max_age_ns)
            return blocked("telemetry_unqualified_or_unavailable");
        if (request.target != Target::full) {
            if (!c.end_threshold || (request.target == Target::band80_90 && !c.start_threshold))
                return blocked("autonomous_thresholds_unavailable");
            if (!finite(s.soc) || *s.soc < 0 || *s.soc > 100) return blocked("soc_unavailable");
            p.end_percent = request.target == Target::cap80 ? 80 : 90;
            if (request.target == Target::band80_90) {
                p.start_percent = 80;
                if (*s.soc >= 90) soc_charge_ = false;
                else if (*s.soc <= 80) soc_charge_ = true;
                // In the band after restart, let autonomous firmware thresholds decide.
                p.soc_demand = !soc_charge_ ? "firmware_hysteresis" : *soc_charge_ ? "charge" : "hold";
            } else p.soc_demand = *s.soc >= 80 ? "hold" : "charge";
        }
        if (request.rate != Rate::firmware) {
            if (!valid(e) || e.epoch != s.epoch) return blocked("electrical_profile_unqualified");
            if (!c.battery_current || !c.battery_voltage) return blocked("battery_cc_cv_controls_unavailable");
            if (!finite(s.temperature_deci_c) || !finite(s.battery_uv) || *s.battery_uv <= 0 ||
                !finite(s.battery_ua) || !finite(s.available_battery_uw) || *s.available_battery_uw <= 0)
                return blocked("electrical_measurements_unavailable");
            const ThermalBand* band = nullptr;
            for (const auto& b : e.bands)
                if (*s.temperature_deci_c >= b.min_deci_c && *s.temperature_deci_c < b.max_deci_c) { band = &b; break; }
            if (!band || !band->max_battery_ua) return blocked("firmware_thermal_inhibit_required");
            if (*s.battery_uv > band->max_battery_uv) return blocked("battery_voltage_above_profile");
            const double power_ua = *s.available_battery_uw * 1000000.0 / *s.battery_uv;
            if (!std::isfinite(power_ua) || power_ua < 1) return blocked("source_power_unavailable");
            const uint32_t ceiling = std::min({e.pack_ua, e.path_ua, e.firmware_ua, band->max_battery_ua,
                request.rate == Rate::slow ? e.slow_ua : e.pack_ua});
            uint32_t current = static_cast<uint32_t>(std::min<double>(ceiling, power_ua));
            uint32_t voltage = band->max_battery_uv;
            if (!current) return blocked("source_power_unavailable");
            // Decreases are immediate; increases require a stable profile band and dwell.
            if (candidate_ua_ != current || candidate_uv_ != voltage) {
                stable_since_ns_ = s.boottime_ns; candidate_ua_ = current; candidate_uv_ = voltage;
            }
            if (previous_ua_ && previous_uv_ && (current > *previous_ua_ || voltage > *previous_uv_)) {
                if (s.net_discharging || *s.battery_ua < 0 || s.boottime_ns - stable_since_ns_ < e.increase_dwell_ns) {
                    current = std::min(current, *previous_ua_); voltage = std::min(voltage, *previous_uv_);
                } else {
                    current = static_cast<uint32_t>(std::min<uint64_t>(current, uint64_t(*previous_ua_) + e.increase_step_ua));
                    stable_since_ns_ = s.boottime_ns;
                }
            }
            previous_ua_ = current; previous_uv_ = voltage;
            p.battery_current_ua = current; p.battery_voltage_uv = voltage;
        }
        p.eligible = true; p.state = "proposal_only"; p.reason = "actuator_and_readback_required";
        return p;
    }
};
// Metadata-only CLI entry point. -1 means the arguments belong to another handler.
// An unsupported apply returns EX_UNAVAILABLE and never persists an effective mode.
int policy_command(int argc, char** argv);
std::string policy_modes_json();
}
