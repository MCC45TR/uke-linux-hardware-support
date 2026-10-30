// SPDX-License-Identifier: MIT
// Parser-only benchmark: the original desktop program is replaced by fixed input.
#include "plasma.hpp"
#include <cassert>
#include <charconv>
#include <chrono>
#include <ctime>
#include <iostream>
namespace uke {
Result execute(const std::vector<std::string>& args,unsigned seconds,size_t limit) {
    assert(args.size()==2&&args[0]=="/usr/bin/kscreen-doctor"&&args[1]=="-o"&&seconds==5&&limit==65536);
    return {0,"\033[01;32mOutput: \033[0m1 DP-1\nAuto Rotate Policy: incapable\nAutomatic brightness: supported, disabled\n"
        "\033[01;32mOutput: \033[0m2 DSI-1\nAuto Rotate Policy: inTabletMode\nAutomatic brightness: supported, enabled\n"};
}
}
int main(int argc,char** argv) {
    unsigned iterations=2000;
    if(argc==2) {
        const std::string_view input(argv[1]);const auto parsed=std::from_chars(input.data(),input.data()+input.size(),iterations);
        assert(parsed.ec==std::errc{}&&parsed.ptr==input.data()+input.size()&&iterations>=10&&iterations<=20000);
    } else assert(argc==1);
    for(unsigned i=0;i<10;++i)(void)uke::desktop::plasma_status();
    const auto cpu=std::clock();const auto start=std::chrono::steady_clock::now();size_t bytes=0;
    for(unsigned i=0;i<iterations;++i) {
        const auto output=uke::desktop::plasma_status();
        assert(output.find("\"output\":\"DSI-1\"")!=output.npos);bytes+=output.size();
    }
    std::cout<<"{\"iterations\":"<<iterations<<",\"output_bytes\":"<<bytes
        <<",\"cpu_ms\":"<<(1000.0*(std::clock()-cpu)/CLOCKS_PER_SEC)
        <<",\"wall_ms\":"<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<"}\n";
}
