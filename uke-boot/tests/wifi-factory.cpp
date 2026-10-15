// SPDX-License-Identifier: MIT
#define main factory_application_main
#include "../src/wifi-factory.cpp"
#undef main
#include <cassert>

int main() {
    assert(factory_mac("wlan0=001122aAbBcC\n") == "00:11:22:AA:BB:CC");
    assert(factory_mac("wlan1=001122334456\r\nwlan0=001122334455\r\n") == "00:11:22:33:44:55");
    for (const auto* invalid : {"", "wlan0=000000000000", "wlan0=FFFFFFFFFFFF",
            "wlan0=011122334455", "wlan0=00112233445", "wlan0=00112233445G",
            "wlan0=001122334455\nwlan0=001122334456", "wlan0=\nwlan0=001122334455",
            "wlan0=001122334455;evil"}) {
        bool rejected = false;
        try { (void)factory_mac(invalid); } catch (const std::exception&) { rejected = true; }
        assert(rejected);
    }
    bool oversized = false;
    try { (void)factory_mac(std::string(1025, 'x')); } catch (const std::exception&) { oversized = true; }
    assert(oversized);
    std::puts("Factory WLAN parser: valid, CRLF, duplicate, malformed and multicast checks passed");
}
