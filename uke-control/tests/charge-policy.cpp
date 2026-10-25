// SPDX-License-Identifier: MIT
// Synthetic electrical values exercise arithmetic; they are not a Uke profile.
#include "charge-policy.hpp"
#include <cassert>
#include <limits>
#include <sstream>
#include <iostream>
using namespace uke::charge;
int main() {
    Planner planner;
    Sample s;
    s.epoch = "fixture"; s.boottime_ns = s.receipt_ns = 100;
    s.coherent = s.measurements_qualified = s.sink = true; s.fault = false;
    s.soc = 79; s.temperature_deci_c = 200; s.battery_uv = 4000000; s.battery_ua = 200000;
    s.available_battery_uw = 5000000;
    Controls c{true, true, true, true, true, true, "fixture"};
    Envelope e;
    e.qualified = true; e.epoch = "fixture"; e.pack_ua = 2000000; e.path_ua = 1500000;
    e.firmware_ua = 1800000; e.slow_ua = 300000; e.max_battery_uv = 4200000;
    e.increase_step_ua = 100000; e.max_age_ns = 50; e.max_gap_ns = 100; e.increase_dwell_ns = 20;
    e.bands = {{100, 300, 1400000, 4200000}, {300, 400, 500000, 4100000}};
    const auto step = [&](Request r) { s.receipt_ns = ++s.boottime_ns; return planner.evaluate(r, s, e, c); };
    const Request cap{Target::cap80, Rate::firmware}, hold{Target::band80_90, Rate::firmware};
    auto p = step(cap); assert(p.eligible && p.end_percent == 80 && !p.start_percent && p.soc_demand == "charge");
    s.soc = 80; assert(step(cap).soc_demand == "hold");
    s.soc = 85; assert(step(hold).soc_demand == "firmware_hysteresis");
    s.soc = 90; p = step(hold); assert(p.start_percent == 80 && p.end_percent == 90 && p.soc_demand == "hold");
    s.soc = 85; assert(step(hold).soc_demand == "hold");
    s.soc = 80; assert(step(hold).soc_demand == "charge");
    s.soc = 85; assert(step(hold).soc_demand == "charge");
    c.start_threshold = false; p = step(hold); assert(!p.eligible && !p.end_percent);
    c.start_threshold = true; assert(step(hold).soc_demand == "firmware_hysteresis");
    s.soc = 79;
    p = step({Target::cap80, Rate::slow}); assert(p.eligible && p.battery_current_ua == 300000 && p.end_percent == 80);
    p = step({Target::full, Rate::automatic}); assert(p.eligible && p.battery_current_ua == 1250000 && p.battery_voltage_uv == 4200000);
    s.temperature_deci_c = 300; p = step({Target::full, Rate::automatic});
    assert(p.battery_current_ua == 500000 && p.battery_voltage_uv == 4100000);
    s.temperature_deci_c = 200; p = step({Target::full, Rate::automatic}); assert(p.battery_current_ua == 500000 && p.battery_voltage_uv == 4100000);
    s.boottime_ns += 20; p = step({Target::full, Rate::automatic}); assert(p.battery_current_ua == 600000);
    s.battery_ua = -100000; s.boottime_ns += 20;
    p = step({Target::full, Rate::automatic}); assert(p.battery_current_ua == 600000);
    s.battery_ua = 200000;
    s.temperature_deci_c = 400; p = step({Target::full, Rate::automatic});
    assert(!p.eligible && p.reason == "firmware_thermal_inhibit_required" && !p.battery_current_ua);
    s.temperature_deci_c = std::numeric_limits<double>::quiet_NaN(); assert(!step({Target::full, Rate::automatic}).eligible);
    s.temperature_deci_c = 200; s.available_battery_uw = -1; assert(!step({Target::full, Rate::slow}).eligible);
    s.available_battery_uw = 5000000; s.battery_uv = 4300000; assert(!step({Target::full, Rate::slow}).eligible);
    s.battery_uv = 4000000; e.qualified = false; assert(!step({Target::full, Rate::slow}).eligible);
    e.qualified = true; e.bands.push_back({250, 350, 1000000, 4100000}); assert(!step({Target::full, Rate::automatic}).eligible); e.bands.pop_back();
    c.qualified = false; assert(!step(cap).eligible); c.qualified = true;
    c.epoch = "old"; assert(!step(cap).eligible); c.epoch = "fixture";
    e.max_gap_ns = 0; assert(!step(cap).eligible); e.max_gap_ns = 100;
    ++s.boottime_ns; s.receipt_ns = s.boottime_ns - 51;
    p = planner.evaluate(cap, s, e, c); assert(!p.eligible && p.reason == "telemetry_unqualified_or_unavailable");
    assert(planner.evaluate(cap, s, e, c).reason == "sample_time_not_increasing");
    s.suspended = true; assert(!step(cap).eligible); s.suspended = false;
    assert(!planner.evaluate({}, {}, {}, {}).eligible);
    assert(planner.evaluate({}, {}, {}, {}).state == "firmware_managed");
    assert(!preset("80; reboot") && preset("hold80-90"));
    // A preview cannot turn a proposal into applied state.
    std::ostringstream output; auto* original = std::cout.rdbuf(output.rdbuf());
    char exe[] = "uke-control", charge[] = "charge", mode[] = "plan", name[] = "charge80";
    char* argv[] = {exe, charge, mode, name};
    assert(policy_command(4, argv,true) == 0);
    std::cout.rdbuf(original);
    assert(output.str().find("\"applied\":false") != std::string::npos);
    assert(output.str().find("\"effective\":null") != std::string::npos);
    assert(policy_modes_json().find("\"qualification_checked\":false") != std::string::npos);
}
