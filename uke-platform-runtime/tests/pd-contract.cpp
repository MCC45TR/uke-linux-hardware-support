// SPDX-License-Identifier: MIT
#include "../src/battery/pd-contract.hpp"
#include <cassert>
#include <iostream>
using namespace uke::battery::control;
int main() {
    // 9 V / 3 A source PDO; request 2 A operating / 2.5 A maximum.
    const uint32_t pdo = (180U << 10) | 300U, rdo = (1U << 28) | (200U << 10) | 250U;
    auto row = [&](uint32_t p, uint32_t r) {
        return "1 3 100 101 1 " + std::to_string(r) + " " + std::to_string(p) + " 1 1\n";
    };
    auto s = parse_pd_contract(row(pdo, rdo), 102);
    assert(s.valid && s.fixed && s.voltage_uv == 9000000 && s.operating_ua == 2000000 && s.maximum_ua == 2500000);
    assert(pd_contract_report(s).find("\"contract_operating_power_uw\":18000000") != std::string::npos);
    assert(!parse_pd_contract("1 3 100 0 0 0 0 0 0\n", 102).valid);
    assert(!parse_pd_contract(row(pdo, rdo), 99).valid);
    assert(!parse_pd_contract(row(pdo, rdo | (1U << 27)), 102).fixed);
    assert(!parse_pd_contract(row(pdo | (3U << 30), rdo), 102).fixed);
    assert(!parse_pd_contract(row((180U << 10) | 100U, rdo), 102).fixed);
    for (const auto &text : {"", "1 3 100 101 2 0 0 0 0", "1 3 100 101 1 -1 0 1 1",
        "1 3 100 101 1 4294967296 0 1 1", "1 3 100 101 1 0 0 1 1 extra",
        "1 3 100 101 1 0 0 1 8"}) {
        bool refused = false; try { parse_pd_contract(text, 102); } catch (...) { refused = true; }
        assert(refused);
    }
    std::cout << "PD cache decoding, lifecycle, fixed-supply bounds and malformed input checks passed\n";
}
