// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace uke {
inline void require_inactive_watchdogs(const std::filesystem::path& directory="/sys/class/watchdog") {
    if(!std::filesystem::exists(directory))return;
    for(const auto& item:std::filesystem::directory_iterator(directory)) {
        std::ifstream input(item.path()/"state");std::string state,extra;
        if(!(input>>state)||input>>extra)
            throw std::runtime_error("Cannot verify watchdog state; boot a kernel with CONFIG_WATCHDOG_SYSFS=y before offline maintenance");
        if(state!="inactive")
            throw std::runtime_error("Watchdog is active or unknown; offline maintenance requires an inactive watchdog");
    }
}
}
