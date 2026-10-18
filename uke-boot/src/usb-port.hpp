// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace uke::usb {
enum class DataRole { unknown, none, device, host };
enum class PowerRole { unknown, sink, source };
enum class Orientation { unknown, none, normal, reverse };
enum class Lanes { unknown, usb, dp2_usb3, dp4_usb2 };
struct Epoch {
    std::uint64_t firmware{}, attachment{}, contract{}, observation{};
    bool operator==(const Epoch& other) const {
        return firmware == other.firmware && attachment == other.attachment &&
               contract == other.contract && observation == other.observation;
    }
};
struct State {
    Epoch epoch;
    DataRole data{DataRole::unknown};
    PowerRole power{PowerRole::unknown};
    Orientation orientation{Orientation::unknown};
    Lanes lanes{Lanes::unknown};
    bool coherent{}, fresh{}, firmware_ready{}, provider_ready{};
    bool active_storage{}, active_network{}, gadget_bound{};
    bool role_control_qualified{}, source_profile_qualified{}, lane_control_qualified{};
};
struct Request {
    Epoch expected;
    std::optional<DataRole> data;
    std::optional<PowerRole> power;
    std::optional<Lanes> lanes;
};
// A plan authorizes no hardware write and never marks an asynchronous SET verified.
inline std::string plan(const State& state, const Request& request) {
    if (!(state.epoch == request.expected)) return "STALE_GENERATION";
    if (!state.coherent || !state.fresh) return "OBSERVATION_UNVERIFIED";
    if (!state.firmware_ready || !state.provider_ready) return "PROVIDER_NOT_READY";
    if (request.data == DataRole::unknown || request.data == DataRole::none ||
        request.power == PowerRole::unknown || request.lanes == Lanes::unknown)
        return "NO_ELECTRICAL_OFF_CONTRACT";
    const bool data_change = request.data && *request.data != state.data;
    const bool lane_change = request.lanes && *request.lanes != state.lanes;
    const bool power_change = request.power && *request.power != state.power;
    if ((data_change || lane_change) && (state.active_storage || state.active_network))
        return "ACTIVE_IO_REQUIRES_QUIESCE";
    if (data_change && state.gadget_bound) return "GADGET_REQUIRES_TEARDOWN";
    if (data_change && !state.role_control_qualified) return "ROLE_BACKEND_UNQUALIFIED";
    if (power_change && *request.power == PowerRole::source && !state.source_profile_qualified)
        return "SOURCE_PROFILE_UNQUALIFIED";
    if (power_change) return "POWER_SWAP_BACKEND_UNQUALIFIED";
    if (lane_change && !state.lane_control_qualified) return "LANE_BACKEND_UNQUALIFIED";
    return data_change || lane_change ? "PREPARED_NOT_APPLIED" : "NO_CHANGE";
}
struct PanDp {
    Orientation orientation{Orientation::unknown};
    unsigned pin_ordinal{}, allocated_lanes{};
    bool hpd{}, irq{};
};
inline std::optional<PanDp> decode_pan_dp(unsigned orientation, unsigned field) {
    if (field > 255 || orientation > 1) return std::nullopt;
    const unsigned pin = field & 63;
    // OEM ordinals C/E allocate four lanes; D/F allocate two. They are not VDO bits.
    if (pin < 3 || pin > 6 || ((field & 128) && !(field & 64))) return std::nullopt;
    return PanDp{orientation ? Orientation::reverse : Orientation::normal,
                 pin, pin == 3 || pin == 5 ? 4u : 2u, bool(field & 64), bool(field & 128)};
}
inline std::optional<std::uint64_t> dp_payload_kbps(unsigned trained_lanes, unsigned rate_mbps) {
    if ((trained_lanes != 1 && trained_lanes != 2 && trained_lanes != 4) ||
        (rate_mbps != 1620 && rate_mbps != 2700 && rate_mbps != 5400)) return std::nullopt;
    return std::uint64_t(trained_lanes) * rate_mbps * 800;
}
struct DpMode {
    unsigned width{}, height{}, pixel_clock_khz{}, bpc{};
    bool edid_advertised{}, rgb_supported{}, interlaced{};
};
struct DpChoice {
    unsigned lanes{};
    bool superspeed{};
    std::string reason;
};
// Use the complete EDID pixel clock, including blanking; never substitute active pixels * Hz.
inline DpChoice choose_dp_lanes(const DpMode& mode, unsigned rate_mbps,
                                bool dp2_available, bool dp4_available,
                                bool active_usb_io) {
    if (!mode.edid_advertised || !mode.rgb_supported || mode.interlaced ||
        !mode.width || !mode.height || !mode.pixel_clock_khz ||
        (mode.bpc != 8 && mode.bpc != 10))
        return {0, false, "MODE_OR_RGB_DEPTH_UNQUALIFIED"};
    const auto two = dp_payload_kbps(2, rate_mbps);
    const auto four = dp_payload_kbps(4, rate_mbps);
    if (!two || !four) return {0, false, "LINK_RATE_OUTSIDE_UKE_HBR2"};
    const auto required = std::uint64_t(mode.pixel_clock_khz) * mode.bpc * 3;
    if (dp2_available && required < *two) return {2, true, "DP2_USB3_CANDIDATE"};
    if (dp4_available && required < *four) {
        if (active_usb_io) return {0, false, "DP4_REQUIRES_USB_QUIESCE"};
        return {4, false, "DP4_USB2_CANDIDATE"};
    }
    return {0, false, "MODE_EXCEEDS_AVAILABLE_LINK"};
}
struct InputLimits {
    std::optional<unsigned> contract_ua, cable_ua, connector_ua, charger_ua, thermal_ua;
};
inline std::optional<unsigned> input_limit(const InputLimits& limits) {
    if (!limits.contract_ua || !limits.cable_ua || !limits.connector_ua ||
        !limits.charger_ua || !limits.thermal_ua) return std::nullopt;
    return std::min({*limits.contract_ua, *limits.cable_ua, *limits.connector_ua,
                     *limits.charger_ua, *limits.thermal_ua});
}
inline std::optional<std::int64_t> measured_power_uw(std::int64_t uv, std::int64_t ua,
                                                   bool same_reference_and_time) {
    if (!same_reference_and_time || uv < 0 || uv > 50000000 || ua < -10000000 || ua > 10000000)
        return std::nullopt;
    return uv * ua / 1000000;
}
}
