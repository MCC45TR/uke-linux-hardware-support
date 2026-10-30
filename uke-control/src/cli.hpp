// SPDX-License-Identifier: MIT
#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace uke::cli {
inline constexpr const char* version="2.3.2";
struct UsageError:std::runtime_error {using std::runtime_error::runtime_error;};
struct Arguments {std::vector<std::string> words;bool json=false,help=false,version=false;};
Arguments arguments(int argc,char** argv);
std::string help(std::string_view group={});
std::string human(const std::string& data);
void print(const std::string& data,bool json=false,std::string_view title={});
}
