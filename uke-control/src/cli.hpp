// SPDX-License-Identifier: MIT
#pragma once
#include "version.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace uke::cli {
inline constexpr const char* version=application_version;
struct UsageError:std::runtime_error {using std::runtime_error::runtime_error;};
struct Arguments {std::vector<std::string> words;bool json=false,help=false,version=false;};
Arguments arguments(int argc,char** argv);
std::string help(std::string_view group={});
std::vector<std::string> command_names();
std::string human(const std::string& data);
void print(const std::string& data,bool json=false,std::string_view title={});
int error(const std::string& message,bool json,int status);
bool json_requested(int argc,char** argv);
}
