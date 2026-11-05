// SPDX-License-Identifier: MIT
// Private local firmware must be ready before a managed kernel is activated.
#pragma once
#include <array>
#include <charconv>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace uke_firmware {
inline bool dry_action(std::string_view action) {
    for (const auto candidate : {"auto", "transaction", "apply", "apply-current",
                                 "replace-trial", "select-slot", "boot"})
        if (action == candidate) return false;
    for (const auto candidate : {"check", "check-auto", "check-current", "check-trial",
                                 "check-select-slot", "accept", "fallback",
                                 "restore-current", "check-restore-current"})
        if (action == candidate) return true;
    throw std::runtime_error("Unsupported managed-firmware updater action");
}
inline unsigned number(std::string_view value, bool nonzero) {
    if (value.empty()) throw std::runtime_error("Managed-firmware version field is empty");
    unsigned result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if ((value.size() > 1 && value.front() == '0') ||
        parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() ||
        (nonzero && !result))
        throw std::runtime_error("Invalid managed-firmware kernel version");
    return result;
}
inline bool required(const std::map<std::string, std::string> &manifest) {
    const auto target = manifest.find("kernel_release");
    // Empty manifests represent unchanged accepted-recovery paths.
    if (target == manifest.end()) return false;
    const std::string_view release = target->second;
    if (!release.ends_with("-unstable")) return false;
    if (release.size() > 96) throw std::runtime_error("Managed-firmware kernel version is too long");
    const auto delimiter = release.find('-');
    if (delimiter == std::string_view::npos)
        throw std::runtime_error("Managed-firmware kernel revision is missing");
    const auto version = release.substr(0, delimiter);
    std::array<unsigned, 3> components{};
    size_t position = 0;
    for (size_t index = 0; index < components.size(); ++index) {
        const auto end = version.find('.', position);
        if ((index < 2) != (end != std::string_view::npos))
            throw std::runtime_error("Invalid managed-firmware kernel version fields");
        components[index] = number(version.substr(position,
            end == std::string_view::npos ? end : end - position), index == 0);
        position = end == std::string_view::npos ? version.size() : end + 1;
    }
    const auto revision = number(release.substr(delimiter + 1,
        release.size() - delimiter - 1 - std::string_view("-unstable").size()), true);
    const auto kernel = std::tuple{components[0], components[1], components[2]};
    // 11003 is the first qualified permanent-PAS cohort. Older recovery stays
    // unchanged; a future kernel version must also satisfy the provider's policy.
    return kernel > std::tuple{7u, 3u, 1u} ||
        (kernel == std::tuple{7u, 3u, 1u} && revision >= 11003);
}
template<class Prepare, class Verify, class Activate>
decltype(auto) with_prepared(const std::map<std::string, std::string> &manifest,
                             bool dry, Prepare prepare, Verify verify, Activate activate) {
    if (required(manifest)) {
        const auto &release = manifest.at("kernel_release");
        if (!dry) prepare(release);
        verify(release);
    }
    return activate();
}
} // namespace uke_firmware
