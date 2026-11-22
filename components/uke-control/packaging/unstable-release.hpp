// SPDX-License-Identifier: MIT
// Canonical semantic version and positive release; exact RPM/source admission
// remains the responsibility of each caller.
#pragma once
#include <array>
#include <charconv>
#include <string_view>
namespace uke_release {
inline bool unstable(std::string_view release) {
    constexpr std::string_view suffix = "-unstable";
    if (!release.ends_with(suffix)) return false;
    release.remove_suffix(suffix.size());
    const auto split = release.find('-');
    if (split == release.npos || release.find('-', split + 1) != release.npos) return false;
    const auto number = [](std::string_view text, unsigned &value) {
        if (text.empty() || (text.size() > 1 && text.front() == '0') ||
            text.find_first_not_of("0123456789") != text.npos) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        return result.ec == std::errc() && result.ptr == text.data() + text.size();
    };
    unsigned revision = 0;
    if (!number(release.substr(split + 1), revision) || !revision) return false;
    auto version = release.substr(0, split);
    std::array<unsigned, 3> parts{};
    for (unsigned i = 0; i < parts.size(); ++i) {
        const auto dot = version.find('.');
        if ((i < 2) != (dot != version.npos) || !number(version.substr(0, dot), parts[i])) return false;
        if (dot != version.npos) version.remove_prefix(dot + 1);
    }
    return parts >= std::array<unsigned, 3>{7, 3, 1};
}
} // namespace uke_release
