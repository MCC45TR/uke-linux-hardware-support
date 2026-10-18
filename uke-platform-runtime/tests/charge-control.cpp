// SPDX-License-Identifier: MIT
// Synthetic host-only contract fixtures. No sysfs, D-Bus or device access.
#include "../src/battery/control.hpp"
#include <cassert>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace uke::battery::control;
namespace {
const std::string sha(64, 'a');
constexpr uint64_t second = 1000000000ULL;
void rejects(const std::function<void()> &action) {
    bool rejected = false; try { action(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
Profile profile() {
    Profile p; p.qualified = true; p.evidence = sha; p.reviewed_ns = second;
    p.expires_ns = 1000 * second; p.lease_ns = 30 * second;
    p.ack_bytes = 12; p.usb_type = 1; p.may_enable = true; p.standard_clients = true;
    p.temperature_min = 100; p.temperature_max = 400; p.voltage_min_uv = 3000000; p.voltage_max_uv = 4400000;
    p.current_abs_max_ua = 3000000; p.cap_resume_percent = 70;
    p.current_control = true; p.minimum_ua = 100000; p.maximum_ua = 2000000;
    p.source_budget_ua = 1500000; p.slow_ua = 500000; p.restore_ua = 1000000;
    p.increase_step_ua = 100000; p.increase_dwell_ns = 10 * second; p.measurement_max_age_ns = 5 * second;
    p.bands = {{100, 350, 2000000, 4400000}, {350, 400, 500000, 4300000}};
    return p;
}
State sample(uint64_t time = 2 * second) {
    State s; s.compiled = true; s.current_compiled = true; s.instance = 42;
    s.generation = 7; s.firmware = 2; s.source = 3; s.invalidation_ns = second / 2;
    s.live = true; s.standard_clients = true; s.evidence = sha; s.expires_ns = 32 * second;
    s.enabled = true; s.start = 70; s.end = 80; s.verified = true; s.verified_ns = time;
    s.current_admitted = true; s.current_verified = true; s.current_verified_ns = time; s.current_ua = 1000000;
    s.measurement_valid = true; s.measurement_ns = time; s.temperature_deciC = 250;
    s.voltage_uv = 4000000; s.measured_current_ua = 500000;
    return s;
}
class Fake final : public Io {
public:
    State value = sample();
    std::vector<std::string> writes;
    bool mismatch = false, delivery_error = false, change_source = false;
    State state() override { return value; }
    void write(const std::string &name, const std::string &record) override {
        writes.push_back(name);
        if (name == "uke_charge_lease") {
            if (record == "revoke\n") { value.live = false; return; }
            value.live = true; value.evidence = sha; value.expires_ns = 32 * second; return;
        }
        if (name == "uke_charge_current_profile") { value.current_admitted = true; return; }
        if (name == "uke_charge_refresh") return;
        if (delivery_error) throw std::runtime_error("synthetic_delivery_ambiguous");
        if (name == "uke_charge_current_ua") {
            if (!mismatch) value.current_ua = uint32_t(std::stoul(record));
        } else if (name == "uke_charge_thresholds") {
            unsigned first, last; std::istringstream in(record); in >> first >> last;
            if (!mismatch) { value.start = first; value.end = last; value.enabled = first || last != 100; }
        }
        if (change_source) ++value.source;
    }
};
}
int main() {
    // Strict metadata shape; unsigned overflow/sign and extra tokens rejected.
    const std::string text = "1 1 1 7 2 3 42 32000000000 1 70 80 1 2000000000 0 1 " + sha +
        " 1 1 1000000 1 2000000000 250 4000000 500000 2000000000 1 500000000\n";
    assert(parse_state(text).current_ua == 1000000);
    rejects([&] { parse_state(text + " extra"); });
    auto negative = text; negative.replace(0, 1, "-1"); rejects([&] { parse_state(negative); });
    auto overflow = text; overflow.replace(6, 1, "4294967296"); rejects([&] { parse_state(overflow); });
    {
        Context c; c.provider_verified = true; c.boottime_ns = 2 * second; c.state = sample();
        auto p = profile();
        auto a = inspect_admission(c, p);
        assert(a.profile_qualified && a.threshold_ready && a.automatic_ready && a.standard_client_ready);
        p.current_control = false;
        a = inspect_admission(c, p); assert(a.threshold_ready && !a.automatic_ready);
        p = profile(); p.qualified = false; p.reason = "synthetic_unqualified";
        a = inspect_admission(c, p); assert(!a.profile_qualified && !a.threshold_ready && a.reason == p.reason);
        p = profile(); c.state.evidence = std::string(64, 'b');
        a = inspect_admission(c, p); assert(a.profile_qualified && !a.threshold_ready);
        c.state = sample(); c.state.standard_clients = false;
        assert(!inspect_admission(c, p).threshold_ready);
        c.state = sample(); c.state.current_admitted = false;
        assert(!inspect_admission(c, p).automatic_ready);
        c.state = sample(); c.state.live = false;
        assert(!inspect_admission(c, p).threshold_ready); // Unexpired foreign lease.
        c.state.expires_ns = 0;
        a = inspect_admission(c, p); assert(a.threshold_ready && a.automatic_ready);
        c.state.invalidation_ns = p.reviewed_ns;
        assert(!inspect_admission(c, p).threshold_ready);
        c.state = sample(); c.boottime_ns = p.expires_ns;
        assert(!inspect_admission(c, p).profile_qualified);
        c.boottime_ns = 2 * second; c.provider_verified = false;
        assert(!inspect_admission(c, p).threshold_ready);
        // These pure decisions take no Io/Session and cannot renew/read/write.
    }
    {
        Fake io; auto p = profile(); p.qualified = false; Session s(io, p, io.value);
        rejects([&] { s.thresholds(70, 80, 2 * second); }); assert(io.writes.empty());
    }
    {
        Fake io; Session s(io, profile(), io.value);
        assert(s.thresholds(80, 90, 2 * second).end == 90);
        assert(!s.thresholds(0, 100, 2 * second).enabled);
        assert(s.current(500000, 2 * second).current_ua == 500000);
        assert(io.writes.size() == 3); // Borrowed broker, no permission revocation.
    }
    for (int fault = 0; fault != 3; ++fault) {
        Fake io; io.mismatch = fault == 0; io.delivery_error = fault == 1; io.change_source = fault == 2;
        Session s(io, profile(), io.value);
        rejects([&] { s.current(500000, 2 * second); });
        assert(s.write_attempted() && io.writes.size() == 1); // Never replay a SET.
    }
    {
        Fake io; auto p = profile(); Session s(io, p, io.value);
        io.value.invalidation_ns = p.reviewed_ns;
        rejects([&] { s.refresh(2 * second); }); assert(io.writes.empty());
    }
    {
        Fake io; Session s(io, profile(), io.value);
        ++io.value.source; rejects([&] { s.refresh(2 * second); }); assert(io.writes.empty());
    }
    auto p = profile(); auto s = sample(); Automatic policy;
    assert(policy.propose(p, s, 2 * second) == 1000000);
    s.measurement_ns = 12 * second; s.current_verified_ns = 12 * second; s.expires_ns = 42 * second;
    assert(policy.propose(p, s, 12 * second) == 1100000); // Bounded rise after dwell.
    s.temperature_deciC = 350;
    assert(policy.propose(p, s, 13 * second) == 500000); // Immediate thermal reduction.
    s.temperature_deciC = 250; s.measured_current_ua = -1000;
    assert(policy.propose(p, s, 14 * second) == 1000000); // No rise during discharge.
    s.measured_current_ua = 1000; s.voltage_uv = 4500000;
    rejects([&] { policy.propose(p, s, 14 * second); });
    s.voltage_uv = 4000000; s.measurement_ns = second;
    rejects([&] { current_ceiling(p, s, 14 * second); });
    s = sample(); s.temperature_deciC = 400;
    rejects([&] { current_ceiling(p, s, 2 * second); });
    s = sample(); s.temperature_deciC = 350; s.voltage_uv = 4350000;
    rejects([&] { current_ceiling(p, s, 2 * second); });
    p.bands[0].maximum_ua = 0; s = sample();
    rejects([&] { current_ceiling(p, s, 2 * second); });
}
