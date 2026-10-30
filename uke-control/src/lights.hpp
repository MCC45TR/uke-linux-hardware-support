// SPDX-License-Identifier: MIT
#pragma once
#include <string>
struct sd_bus;
namespace uke::lights {
sd_bus* client_bus();
unsigned color(const std::string& text);
unsigned percent(const std::string& text);
int command(bool rgb,int argc,char** argv,bool as_json=false);
}
