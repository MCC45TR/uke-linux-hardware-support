// SPDX-License-Identifier: MIT
#include "rgb.hpp"
#include <cassert>
#include <functional>
#include <iostream>
static void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
int main() {
    using namespace uke::rgb;
    assert((parse_hex("#ffff00")==Color{255,255,0}));
    assert((parse_hex("00Ff80")==Color{0,255,128}));
    for(unsigned r=0;r<=255;++r)assert((parse_hex(hex({r,255-r,128}))==Color{r,255-r,128}));
    for(const std::string text:{"#fff","#ffff0000","#GG0000","#ff0000;id","-00000","#ff 000",""})
        refuses([&]{(void)parse_hex(text);});
    refuses([]{(void)hex({256,0,0});});
    std::cout<<"HEX RGB: yellow, intermediate components, round trips and malformed-input refusals passed; no device write\n";
}
