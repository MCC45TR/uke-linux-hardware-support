// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <linux/rfkill.h>
#include <optional>
#include <string>
#include <vector>
namespace uke::nfc {
struct Radio {
    std::string name;
    std::filesystem::path path;
    std::optional<unsigned> index;
    std::optional<bool> soft,hard;
};
std::vector<Radio> radios(const std::filesystem::path& sys="/sys");
rfkill_event request(const std::vector<Radio>& radios,bool enabled);
std::string snapshot(const std::filesystem::path& sys="/sys");
}
