// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace uke::display {
std::string front_color_snapshot(std::string_view payload, std::uint64_t now_ns);
std::string front_color_status();
}
