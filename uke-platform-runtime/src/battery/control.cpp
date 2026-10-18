// SPDX-License-Identifier: MIT
#include "control.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace uke::battery::control {
namespace {
[[noreturn]] void fail(const char *reason) { throw std::runtime_error(reason); }
bool digest(const std::string &s) {
    return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == std::string::npos;
}
bool same(const State &a, const State &b) {
    return a.compiled && b.compiled && a.instance && a.instance == b.instance &&
        a.firmware == b.firmware && a.source == b.source;
}
template <class T> T decimal(const std::string &text) {
    T value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) fail("control_state_number_invalid");
    return value;
}
}
State parse_state(const std::string &text) {
    if (text.size() > 4096) fail("control_state_size_invalid");
    State s; std::array<std::string, 27> token;
    std::istringstream in(text);
    for (auto &value : token) if (!(in >> value)) fail("control_state_fields_missing");
    const auto version = decimal<unsigned>(token[0]), compiled = decimal<unsigned>(token[1]);
    const auto live = decimal<unsigned>(token[2]), enabled = decimal<unsigned>(token[8]);
    const auto verified = decimal<unsigned>(token[11]), standard = decimal<unsigned>(token[14]);
    s.generation = decimal<uint32_t>(token[3]); s.firmware = decimal<uint32_t>(token[4]);
    s.source = decimal<uint32_t>(token[5]); s.instance = decimal<uint64_t>(token[6]);
    s.expires_ns = decimal<uint64_t>(token[7]); s.start = decimal<uint32_t>(token[9]);
    s.end = decimal<uint32_t>(token[10]); s.verified_ns = decimal<uint64_t>(token[12]);
    s.error = decimal<int>(token[13]); s.evidence = token[15];
    const auto cc = decimal<unsigned>(token[16]), ca = decimal<unsigned>(token[17]);
    const auto cv = decimal<unsigned>(token[19]), mv = decimal<unsigned>(token[25]);
    s.current_ua = decimal<uint32_t>(token[18]); s.current_verified_ns = decimal<uint64_t>(token[20]);
    s.temperature_deciC = decimal<int32_t>(token[21]); s.voltage_uv = decimal<uint32_t>(token[22]);
    s.measured_current_ua = decimal<int32_t>(token[23]); s.measurement_ns = decimal<uint64_t>(token[24]);
    s.invalidation_ns = decimal<uint64_t>(token[26]);
    if (version != 1 ||
        compiled > 1 || live > 1 || enabled > 1 || verified > 1 || standard > 1 ||
        cc > 1 || ca > 1 || cv > 1 || mv > 1 || s.current_ua > INT32_MAX ||
        s.start > 100 || s.end > 100 || !(in >> std::ws).eof() ||
        (s.evidence != "none" && !digest(s.evidence))) fail("control_state_invalid");
    s.compiled = compiled; s.live = live; s.enabled = enabled;
    s.verified = verified; s.standard_clients = standard;
    s.current_compiled = cc; s.current_admitted = ca; s.current_verified = cv; s.measurement_valid = mv;
    if ((s.live || s.verified) && (!s.compiled || !s.instance || !digest(s.evidence)))
        fail("control_state_binding_invalid");
    if ((ca && !cc) || (cv && (!ca || !s.current_ua || !s.current_verified_ns)) ||
        (mv && !s.measurement_ns)) fail("control_current_state_invalid");
    return s;
}
Session::Session(Io &io, Profile profile, State binding)
    : io_(io), profile_(std::move(profile)), binding_(std::move(binding)) {}
Session::~Session() { try { revoke(); } catch (...) {} }
void Session::admit(uint64_t now) {
    if (!profile_.qualified || !digest(profile_.evidence) || !now ||
        profile_.expires_ns <= now || profile_.lease_ns < 15000000000ULL ||
        profile_.lease_ns > 30000000000ULL) fail("control_profile_unqualified_or_expired");
    const State s = io_.state();
    if (!same(s, binding_)) fail("control_identity_changed");
    if (!profile_.reviewed_ns || profile_.reviewed_ns <= s.invalidation_ns)
        fail("control_review_invalidated_by_lifecycle");
    if (s.live) {
        if (s.evidence != profile_.evidence || (!owns_lease_ && !s.standard_clients))
            fail("control_lease_other_owner");
        if (!owns_lease_) {
            if (profile_.current_control && (!s.current_compiled || !s.current_admitted))
                fail("control_current_not_admitted");
            return;
        }
    }
    const uint64_t expires = now + std::min(profile_.lease_ns, profile_.expires_ns - now);
    if (expires < now) fail("control_lease_time_overflow");
    std::ostringstream record;
    record << "1 " << s.instance << ' ' << s.generation << ' ' << s.firmware << ' '
        << s.source << ' ' << expires << ' ' << profile_.ack_bytes << ' '
        << profile_.usb_type << ' ' << profile_.may_enable << ' ' << profile_.standard_clients << ' '
        << profile_.temperature_min << ' ' << profile_.temperature_max << ' '
        << profile_.voltage_min_uv << ' ' << profile_.voltage_max_uv << ' '
        << profile_.current_abs_max_ua << ' ' << profile_.cap_resume_percent << ' ' << profile_.evidence
        << ' ' << profile_.reviewed_ns << '\n';
    owns_lease_ = true;
    io_.write("uke_charge_lease", record.str());
    const State next = io_.state();
    if (!same(next, binding_) || !next.live || next.evidence != profile_.evidence)
        fail("control_admission_not_observed");
    if (profile_.current_control && !next.current_admitted) {
        if (!next.current_compiled) fail("control_current_not_compiled");
        std::ostringstream table;
        table << "1 " << profile_.evidence << ' ' << profile_.minimum_ua << ' ' << profile_.maximum_ua
            << ' ' << profile_.source_budget_ua << ' ' << profile_.bands.size();
        for (const auto &band : profile_.bands)
            table << ' ' << band.minimum_deciC << ' ' << band.maximum_deciC << ' ' << band.maximum_ua << ' ' << band.maximum_uv;
        table << '\n'; io_.write("uke_charge_current_profile", table.str());
        const auto admitted = io_.state();
        if (!same(admitted, binding_) || !admitted.live || !admitted.current_admitted || admitted.evidence != profile_.evidence)
            fail("control_current_admission_not_observed");
    }
}
State Session::refresh(uint64_t now) {
    admit(now);
    io_.write("uke_charge_refresh", "1\n");
    const State s = io_.state();
    if (!same(s, binding_) || !s.live || !s.verified || s.error ||
        s.evidence != profile_.evidence || s.verified_ns < now || s.expires_ns <= s.verified_ns)
        fail("control_readback_not_verified");
    if (profile_.current_control && (!s.current_verified || s.current_verified_ns < now ||
        s.current_verified_ns >= s.expires_ns)) fail("control_current_readback_not_verified");
    return s;
}
State Session::current(uint32_t value, uint64_t now) {
    if (!profile_.current_control || value < profile_.minimum_ua || value > profile_.maximum_ua)
        fail("control_current_value_unqualified");
    admit(now); write_attempted_ = true;
    io_.write("uke_charge_current_ua", std::to_string(value) + "\n");
    const auto s = io_.state();
    if (!same(s, binding_) || !s.live || !s.current_verified || s.error || s.current_ua != value ||
        s.evidence != profile_.evidence || s.current_verified_ns < now || s.current_verified_ns >= s.expires_ns)
        fail("control_current_readback_mismatch");
    return s;
}
uint32_t current_ceiling(const Profile &p, const State &s, uint64_t now) {
    if (!p.qualified || !p.current_control || !s.live || !s.current_verified || !s.current_admitted ||
        !s.measurement_valid || s.evidence != p.evidence || s.current_ua < p.minimum_ua ||
        !now || now >= p.expires_ns || now >= s.expires_ns || !s.measurement_ns || s.measurement_ns > now ||
        now - s.measurement_ns > p.measurement_max_age_ns || !s.current_verified_ns ||
        s.current_verified_ns > now || now - s.current_verified_ns > p.measurement_max_age_ns || s.error)
        fail("control_current_observations_unqualified");
    const int64_t measured = s.measured_current_ua;
    if (s.voltage_uv < p.voltage_min_uv || s.voltage_uv > p.voltage_max_uv ||
        (measured < 0 ? -measured : measured) > p.current_abs_max_ua)
        fail("control_current_measurement_outside_limits");
    for (const auto &band : p.bands) {
        if (s.temperature_deciC >= band.minimum_deciC && s.temperature_deciC < band.maximum_deciC) {
            const auto ceiling = std::min({band.maximum_ua, p.maximum_ua, p.source_budget_ua});
            if (s.voltage_uv > band.maximum_uv || !ceiling || ceiling < p.minimum_ua)
                fail("control_current_band_inhibits_changes");
            return ceiling;
        }
    }
    fail("control_current_temperature_outside_bands");
}
uint32_t Automatic::propose(const Profile &p, const State &s, uint64_t now) {
    const auto ceiling = current_ceiling(p, s, now);
    if (!previous_ns_ || now <= previous_ns_ || now - previous_ns_ > p.lease_ns ||
        ceiling != ceiling_ || s.current_ua != baseline_ || s.measured_current_ua < 0)
        stable_since_ = now;
    previous_ns_ = now; ceiling_ = ceiling; baseline_ = s.current_ua;
    if (ceiling < s.current_ua) return ceiling;
    if (s.measured_current_ua < 0 || now - stable_since_ < p.increase_dwell_ns) return s.current_ua;
    stable_since_ = now;
    return uint32_t(std::min<uint64_t>(ceiling, uint64_t(s.current_ua) + p.increase_step_ua));
}
State Session::thresholds(uint32_t start, uint32_t end, uint64_t now) {
    if (start >= end || end > 100) fail("control_threshold_pair_invalid");
    admit(now);
    write_attempted_ = true;
    io_.write("uke_charge_thresholds", std::to_string(start) + " " + std::to_string(end) + "\n");
    const State s = io_.state();
    const bool matches = start == 0 && end == 100 ? !s.enabled : s.enabled && s.start == start && s.end == end;
    if (!same(s, binding_) || !s.live || !s.verified || s.error || !matches ||
        s.evidence != profile_.evidence ||
        s.verified_ns < now || s.expires_ns <= s.verified_ns) fail("control_threshold_readback_mismatch");
    return s;
}
void Session::revoke() {
    if (owns_lease_) { io_.write("uke_charge_lease", "revoke\n"); owns_lease_ = false; }
}
}
