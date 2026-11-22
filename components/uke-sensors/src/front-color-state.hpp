// SPDX-License-Identifier: MIT
#pragma once
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace uke::color {
inline constexpr std::uint64_t freshness_window_ns = 30'000'000'000ULL;
enum class Error { starting, awaiting_sample, metadata, transport, invalid_sample, none };
inline const char *error_name(Error error) {
    switch (error) {
    case Error::starting: return "starting";
    case Error::awaiting_sample: return "awaiting_sample";
    case Error::metadata: return "metadata_unavailable";
    case Error::transport: return "transport_unavailable";
    case Error::invalid_sample: return "invalid_sample";
    case Error::none: return "";
    }
    return "invalid_state";
}
struct State {
    bool connected = false;
    Error error = Error::starting;
    std::vector<float> values;
    std::uint64_t timestamp_ticks = 0, received_boottime_ns = 0;
    unsigned status = 0;

    void clear(Error reason) {
        connected = false; error = reason; values.clear();
        timestamp_ticks = received_boottime_ns = 0; status = 0;
    }
    void connect() { clear(Error::awaiting_sample); connected = true; }
    void accept(std::uint64_t ticks, unsigned wire_status, const std::vector<float> &raw, std::uint64_t now_ns) {
        if (!connected || ticks == 0 || wire_status > 3 || raw.empty() || raw.size() > 16 ||
            (!values.empty() && (ticks <= timestamp_ticks || now_ns < received_boottime_ns)))
            throw std::runtime_error("Invalid front-color sample metadata");
        for (auto number : raw) if (!std::isfinite(number))
            throw std::runtime_error("Invalid front-color sample value");
        values = raw; timestamp_ticks = ticks; status = wire_status;
        received_boottime_ns = now_ns; error = Error::none;
    }
    std::string json(std::uint64_t now_ns) const {
        const bool has_sample = !values.empty();
        const bool fresh = connected && has_sample && now_ns >= received_boottime_ns &&
                           now_ns - received_boottime_ns <= freshness_window_ns;
        const bool available = fresh && status > 0 && error == Error::none;
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::boolalpha << std::setprecision(std::numeric_limits<float>::max_digits10)
            << "{\"schema\":\"org.uke.sensors.front-color.v1\",\"version\":1,\"data_type\":\"cct_front\""
            << ",\"connected\":" << connected << ",\"has_sample\":" << has_sample
            << ",\"fresh\":" << fresh << ",\"available\":" << available
            << ",\"error\":\"" << error_name(error) << "\",\"values\":[";
        for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << values[i]; }
        out << "],\"status\":" << (has_sample ? std::to_string(status) : "null")
            << ",\"timestamp_ticks\":" << (has_sample ? std::to_string(timestamp_ticks) : "null")
            << ",\"dsp_clock_hz\":null,\"dsp_clock_mapping\":\"unverified\""
            << ",\"received_boottime_ns\":" << (has_sample ? std::to_string(received_boottime_ns) : "null")
            << ",\"published_boottime_ns\":" << now_ns
            << ",\"freshness_window_ns\":" << freshness_window_ns
            << ",\"units\":\"unverified\",\"calibration\":\"unverified\"}\n";
        return out.str();
    }
};
} // namespace uke::color
