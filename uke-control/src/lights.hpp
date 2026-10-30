// SPDX-License-Identifier: MIT
#pragma once
#include <string>
namespace uke::lights {
unsigned color(const std::string& text);
unsigned percent(const std::string& text);
int command(bool rgb,int argc,char** argv,bool as_json=false);
}
