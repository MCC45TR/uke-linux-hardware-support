// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
namespace uke::sensors {
struct Reading {std::string source,label,unit,status;std::optional<double> value;std::string text;};
std::vector<Reading> sysfs_snapshot(const std::filesystem::path& sys="/sys");
std::string json(const std::vector<Reading>& rows);
int command(int argc,char** argv);
}
