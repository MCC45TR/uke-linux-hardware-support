// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <string>
#include <string_view>
namespace uke::usb {
inline bool typec_port_name(std::string_view name) {
    return name.starts_with("port")&&name.size()>4&&name.find_first_not_of("0123456789",4)==name.npos;
}
// The root argument supports offline fixtures; this function performs no writes.
std::string snapshot(const std::filesystem::path& root);
}
