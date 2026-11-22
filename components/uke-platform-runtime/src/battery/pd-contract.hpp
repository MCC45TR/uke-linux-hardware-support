// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

namespace uke::battery::control {
struct PdContract {
    uint64_t generation = 0, status_ns = 0, pdos_ns = 0;
    uint32_t rdo = 0, pdo = 0, index = 0, count = 0;
    bool valid = false, fixed = false;
    uint32_t voltage_uv = 0, operating_ua = 0, maximum_ua = 0;
};
inline PdContract parse_pd_contract(const std::string &text, uint64_t now) {
    if (text.size() > 512 || text.find('\0') != std::string::npos)
        throw std::runtime_error("pd_contract_size_invalid");
    PdContract s; unsigned version, valid;
    std::istringstream in(text);
    // Reject signs and overflow before streaming unsigned values.
    if (text.find_first_not_of("0123456789 \n\t") != std::string::npos ||
        !(in >> version >> s.generation >> s.status_ns >> s.pdos_ns >> valid >>
          s.rdo >> s.pdo >> s.index >> s.count) || !(in >> std::ws).eof() ||
        version != 1 || valid > 1 || s.count > 7)
        throw std::runtime_error("pd_contract_schema_invalid");
    s.valid = valid && s.generation && s.status_ns && s.pdos_ns >= s.status_ns &&
        s.pdos_ns <= now && s.index && s.index <= s.count &&
        s.index == ((s.rdo >> 28) & 7) && s.pdo;
    // GiveBack and non-fixed requests have different semantics; keep them raw.
    s.fixed = s.valid && !(s.pdo >> 30) && !(s.rdo & (1U << 27));
    if (s.fixed) {
        const uint32_t advertised_ma = (s.pdo & 1023) * 10;
        s.voltage_uv = ((s.pdo >> 10) & 1023) * 50000;
        s.operating_ua = ((s.rdo >> 10) & 1023) * 10000;
        s.maximum_ua = (s.rdo & 1023) * 10000;
        if (!s.voltage_uv || !s.operating_ua || s.operating_ua > s.maximum_ua ||
            s.maximum_ua / 1000 > advertised_ma) s.fixed = false;
    }
    return s;
}
inline std::string pd_contract_report(const PdContract &s) {
    std::string out = "{\"available\":" + std::string(s.valid ? "true" : "false") +
        ",\"source\":\"ucsi_firmware_cache\",\"electrical_measurement\":false,"
        "\"negotiated_revision\":null,\"status_generation\":" + std::to_string(s.generation) +
        ",\"status_received_boottime_ns\":" + std::to_string(s.status_ns) +
        ",\"pdo_received_boottime_ns\":" + std::to_string(s.pdos_ns);
    out += ",\"pdo\":" + (s.valid ? std::to_string(s.pdo) : "null");
    out += ",\"rdo\":" + (s.valid ? std::to_string(s.rdo) : "null");
    out += ",\"fixed_supply_decoded\":" + std::string(s.fixed ? "true" : "false");
    out += ",\"voltage_uv\":" + (s.fixed ? std::to_string(s.voltage_uv) : "null");
    out += ",\"operating_current_ua\":" + (s.fixed ? std::to_string(s.operating_ua) : "null");
    out += ",\"maximum_current_ua\":" + (s.fixed ? std::to_string(s.maximum_ua) : "null");
    out += ",\"contract_operating_power_uw\":" +
        (s.fixed ? std::to_string(uint64_t(s.voltage_uv) * s.operating_ua / 1000000) : "null");
    return out + ",\"physical_acceptance\":false}";
}
}
