// SPDX-License-Identifier: MIT
#pragma once
#include "backend.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace uke::links {
std::string display_bits_status(const std::filesystem::path& root);
std::vector<std::filesystem::path> lane_controls(const std::filesystem::path& root);
std::vector<std::filesystem::path> compression_controls(const std::filesystem::path& root);
std::string compression_status(const std::filesystem::path& root);
std::filesystem::path usb_device(const std::filesystem::path& root,const std::string& name);
std::filesystem::path ethernet_device(const std::filesystem::path& root,const std::string& name);
std::vector<std::string> ethernet_arguments(const std::string& name,const std::string& profile,const std::string& capabilities);
bool ethernet_matches(const std::string& name,const std::string& profile,const std::string& capabilities);
std::string ethernet_status(const std::string& name);
Result lane_limit(const std::string& lanes);
Result compression(const std::string& policy);
Result usb_power(const std::string& name,const std::string& policy);
Result ethernet_profile(const std::string& name,const std::string& profile);
}
