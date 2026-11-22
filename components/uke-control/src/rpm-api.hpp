// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <string_view>
namespace uke {
inline bool rpm_api_matches(std::string_view output, std::string_view capability,
                            std::string_view version) {
    const std::string prefix = std::string(capability) + '\t';
    const std::string expected = prefix + std::string(version);
    unsigned matches = 0;
    while (!output.empty()) {
        const auto end = output.find('\n');
        if (end == std::string_view::npos) return false;
        const auto line = output.substr(0, end);
        if (line.starts_with(prefix)) {
            if (line != expected || ++matches != 1) return false;
        }
        output.remove_prefix(end + 1);
    }
    return matches == 1;
}
}
