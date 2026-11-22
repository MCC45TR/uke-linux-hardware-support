// SPDX-License-Identifier: MIT
#pragma once
#include <string>
namespace uke::diagnostic {
int command(int argc,char** argv,bool as_json=false);
std::string collect(const std::string& topic);
}
