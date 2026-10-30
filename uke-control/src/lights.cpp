// SPDX-License-Identifier: MIT
#include "lights.hpp"
#include "rgb.hpp"
#include "torch.hpp"
#include <charconv>
#include <stdexcept>
namespace uke::lights {
static unsigned number(const std::string& value,unsigned maximum) {
    unsigned n=0;
    const auto [end,error]=std::from_chars(value.data(),value.data()+value.size(),n);
    if(value.empty() || error!=std::errc{} || end!=value.data()+value.size() || n>maximum)
        throw std::runtime_error("Light value out of range");
    return n;
}
unsigned color(const std::string& value){return number(value,255);}
unsigned percent(const std::string& value){return number(value,100);}
int command(bool rgb,int argc,char** argv,bool as_json) {
    return rgb?uke::rgb::command(argc,argv,as_json):uke::torch::command(argc,argv,as_json);
}
}
