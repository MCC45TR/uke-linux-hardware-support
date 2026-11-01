// SPDX-License-Identifier: MIT
#include "../src/usb-port.hpp"
#include <cassert>
#include <climits>
using namespace uke::usb;
int main() {
    State s;
    s.epoch = {1, 2, 3, 4};
    Request r{s.epoch, DataRole::host, PowerRole::sink, std::nullopt};
    assert(plan(s, r) == "OBSERVATION_UNVERIFIED");
    s.coherent = s.fresh = s.firmware_ready = s.provider_ready = true;
    s.power = PowerRole::sink;
    assert(plan(s, r) == "ROLE_BACKEND_UNQUALIFIED");
    s.role_control_qualified = true;
    assert(plan(s, r) == "PREPARED_NOT_APPLIED"); // host + sink is an independent-role target.
    s.active_storage = true;
    assert(plan(s, r) == "ACTIVE_IO_REQUIRES_QUIESCE");
    s.active_storage = false;
    s.gadget_bound = true;
    assert(plan(s, r) == "GADGET_REQUIRES_TEARDOWN");
    s.gadget_bound = false;
    s.epoch.attachment++;
    assert(plan(s, r) == "STALE_GENERATION");
    r.expected = s.epoch;
    r.data = DataRole::none;
    assert(plan(s, r) == "NO_ELECTRICAL_OFF_CONTRACT");
    r.data = DataRole::host;
    r.power = PowerRole::source;
    assert(plan(s, r) == "SOURCE_PROFILE_UNQUALIFIED");
    s.source_profile_qualified = true;
    assert(plan(s, r) == "POWER_SWAP_BACKEND_UNQUALIFIED");
    assert(!decode_pan_dp(2, 3 | 64));
    assert(!decode_pan_dp(0, 0));
    assert(!decode_pan_dp(0, 2 | 64));
    assert(!decode_pan_dp(0, 3 | 128));
    assert(decode_pan_dp(0, 3 | 64)->allocated_lanes == 4);
    assert(decode_pan_dp(1, 4 | 192)->orientation == Orientation::reverse);
    assert(decode_pan_dp(1, 4 | 192)->allocated_lanes == 2);
    assert(dp_payload_kbps(2, 5400) == 8640000);
    assert(dp_payload_kbps(4, 5400) == 17280000);
    assert(dp_payload_kbps(1, 2700) == 2160000); // Trained capacity may be less than allocation.
    assert(!dp_payload_kbps(3, 5400) && !dp_payload_kbps(4, 8100));
    // Representative reduced-blanking clock, not an assertion about the user's EDID.
    DpMode qhd{2560, 1440, 304250, 10, true, true, false};
    assert(choose_dp_lanes(qhd, 5400, true, true, false).lanes == 4);
    assert(choose_dp_lanes(qhd, 5400, true, false, false).lanes == 0);
    assert(choose_dp_lanes(qhd, 5400, true, true, true).reason == "DP4_REQUIRES_USB_QUIESCE");
    qhd.bpc = 8;
    assert(choose_dp_lanes(qhd, 5400, true, true, false).superspeed);
    qhd.edid_advertised = false;
    assert(choose_dp_lanes(qhd, 5400, true, true, false).lanes == 0);
    qhd.edid_advertised = true;
    assert(choose_dp_lanes(qhd, 8100, true, true, false).lanes == 0);
    qhd.pixel_clock_khz = UINT_MAX;
    assert(choose_dp_lanes(qhd, 5400, true, true, false).lanes == 0);
    assert(!input_limit({500000, std::nullopt, 3000000, 3000000, 2000000}));
    assert(input_limit({500000, 3000000, 3000000, 3000000, 2000000}) == 500000);
    assert(input_limit({500000, 3000000, 3000000, 3000000, 0}) == 0);
    assert(measured_power_uw(5000000, 500000, true) == 2500000);
    assert(measured_power_uw(4000000, -500000, true) == -2000000);
    assert(!measured_power_uw(5000000, 500000, false));
    assert(!measured_power_uw(LLONG_MAX, LLONG_MAX, true));
}
