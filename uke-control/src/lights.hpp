// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <filesystem>
#include <string>
namespace uke::lights {
struct Source {
    std::filesystem::path path;
    std::string reason;
    unsigned brightness=0;
    std::string order;
    std::array<unsigned,3> rgb{};
};
Source discover(bool rgb, const std::filesystem::path& sys="/sys");
std::string snapshot(const std::filesystem::path& sys="/sys");
unsigned color(const std::string& text);
unsigned percent(const std::string& text);
unsigned level(unsigned percent, unsigned maximum);
std::array<unsigned,3> colors(const std::string& order, const std::string& values);
std::string intensities(const std::string& order, const std::array<unsigned,3>& rgb);
int command(bool rgb, int argc, char** argv);
}
