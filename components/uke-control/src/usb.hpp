// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <string>
namespace uke::usb {
// The root argument supports offline fixtures; this function performs no writes.
std::string snapshot(const std::filesystem::path& root);
}
