// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>
#include <limits>

namespace uke::battery {
struct FixedSource { uint32_t voltage_uv, current_ua; bool pps = false, epr = false; };
struct SourceLimits {
    bool qualified = false, firmware_owner_verified = false, sink = false, cable_verified = false;
    uint64_t generation = 0;
    uint32_t cable_ua = 0, board_ua = 0, path_ua = 0, profile_ua = 0, thermal_ua = 0;
    uint64_t power_uw = 0;
    std::vector<uint32_t> allowed_vbus_uv;
};
struct FixedChoice { uint32_t voltage_uv, current_ua; uint64_t generation; };
// Pure admission arithmetic only. A choice never negotiates or applies power.
inline std::optional<FixedChoice> fixed_choice(const std::vector<FixedSource> &sources,
                                               const SourceLimits &limits, uint64_t generation) {
    if (!limits.qualified || !limits.firmware_owner_verified || !limits.sink || !limits.cable_verified ||
        generation != limits.generation || !limits.power_uw ||
        limits.power_uw > std::numeric_limits<uint64_t>::max() / 1000000ULL) return {};
    std::optional<FixedChoice> choice; uint64_t best_power = 0;
    for (const auto &source : sources) {
        if (!source.voltage_uv || source.pps || source.epr ||
            std::find(limits.allowed_vbus_uv.begin(), limits.allowed_vbus_uv.end(), source.voltage_uv) == limits.allowed_vbus_uv.end()) continue;
        const uint64_t from_power = limits.power_uw * 1000000ULL / source.voltage_uv;
        const uint64_t current = std::min<uint64_t>({source.current_ua, limits.cable_ua, limits.board_ua,
            limits.path_ua, limits.profile_ua, limits.thermal_ua, from_power});
        const uint64_t power = uint64_t(source.voltage_uv) * current / 1000000ULL;
        if (current && power > best_power) { choice = FixedChoice{source.voltage_uv, uint32_t(current), generation}; best_power = power; }
    }
    return choice;
}
}
