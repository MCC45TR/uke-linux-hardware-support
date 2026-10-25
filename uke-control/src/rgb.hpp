// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <string>
#include <string_view>
namespace uke::rgb {
using Color=std::array<unsigned,3>;
inline constexpr const char* bus_name="org.senemos.Uke.Rgb";
inline constexpr const char* object_path="/org/senemos/Uke/Rgb";
inline constexpr const char* interface="org.senemos.Uke.Rgb1";
Color parse_hex(std::string_view text);
std::string hex(Color color);
int command(int argc,char** argv);
}
