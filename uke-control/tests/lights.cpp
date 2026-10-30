// SPDX-License-Identifier: MIT
#include "lights.hpp"
#include <cassert>
#include <functional>
#include <iostream>
static void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
int main() {
    using namespace uke::lights;
    assert(color("255")==255 && color("0")==0);
    assert(percent("100")==100 && percent("0")==0);
    for(const std::string value:{"101","-1","1;reboot","+1"," 1",""})refuses([&]{(void)percent(value);});
    for(const std::string value:{"-1","256","1;reboot","+1"," 1","1x","","4294967296"})refuses([&]{(void)color(value);});
    std::cout<<"Light request input bounds passed; hardware policy belongs to the kernel\n";
}
