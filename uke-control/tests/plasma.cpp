// SPDX-License-Identifier: MIT
#include "plasma.hpp"
#include <cassert>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <vector>
static std::vector<std::vector<std::string>> calls;
static uke::Result response{0, ""};
static uke::Result mutation{0, ""};
static std::optional<uke::Result> after;
namespace uke {
Result execute(const std::vector<std::string>& args, unsigned seconds, size_t limit) {
    assert(seconds == 5 && limit == 65536);
    calls.push_back(args);
    if(args.size()==2&&args[0]=="/usr/bin/kscreen-doctor"&&args[1]=="-o")return response;
    if(after)response=*after;
    return mutation;
}
}
int main() {
    assert(setenv("LC_ALL","C.UTF-8",1)==0);
    const std::string panel = "Output: 1 DSI-1 enabled connected\n"
        "\tAuto Rotate Policy: always\n\tAutomatic brightness: supported, disabled\n";
    response.output = "\033[01;31m" + panel + "\033[0mOutput: 2 DP-1 enabled\n"
        "\tAuto Rotate Policy: never\n\tAutomatic brightness: supported, enabled\n";
    assert(uke::desktop::plasma_status().find("\"rotation\":true,\"automatic_brightness\":false") != std::string::npos);
    assert(std::string(std::getenv("LC_ALL"))=="C.UTF-8");
    const std::string never="Output: 1 DSI-1\nAuto Rotate Policy: never\nAutomatic brightness: supported, disabled\n";
    after=uke::Result{0,never};auto count=calls.size();
    assert(uke::desktop::plasma_set("rotation", "off").output.find("\"policy_readback_matched\":true")!=std::string::npos);
    assert(calls.size()==count+3&&calls[count+1].back()=="output.DSI-1.autoRotatePolicy.never");
    after=uke::Result{0,"Output: 1 DSI-1\nAuto Rotate Policy: never\nAutomatic brightness: supported, enabled\n"};
    count=calls.size();uke::desktop::plasma_set("brightness", "on");
    assert(calls.size()==count+3&&calls[count+1].back()=="output.DSI-1.autoBrightness.enable");
    count=calls.size();
    assert(uke::desktop::plasma_set("brightness","on").output.find("\"already_matching\":true")!=std::string::npos);
    assert(calls.size()==count+1);
    response.output = "Output: 1 DP-1\nAuto Rotate Policy: never\nAutomatic brightness: supported, disabled\n"
        "Output: 2 DSI-2\nAuto Rotate Policy: inTabletMode\nAutomatic brightness: supported, enabled\n";
    assert(uke::desktop::plasma_status().find("\"output\":\"DSI-2\",\"rotation\":false,\"automatic_brightness\":true") != std::string::npos);
    assert(uke::desktop::plasma_status().find("\"rotation_policy\":\"inTabletMode\"")!=std::string::npos);
    after=uke::Result{0,"Output: 2 DSI-2\nAuto Rotate Policy: never\nAutomatic brightness: supported, enabled\n"};
    count=calls.size();uke::desktop::plasma_set("rotation","off");
    assert(calls.size()==count+3&&calls[count+1].back()=="output.DSI-2.autoRotatePolicy.never");
    for(const auto& output:{never,panel,std::string("Output: 3 DSI-3\nAuto Rotate Policy: never\n"),std::string("Output: 2 DP-1\n")}) {
        response={0,panel};after=uke::Result{0,output};count=calls.size();
        // A successful setter process cannot establish the requested policy.
        try {uke::desktop::plasma_set("brightness","on");assert(false);}catch(const std::runtime_error&){}
        assert(calls.size()==count+3);
    }
    response={0,panel};after.reset();mutation={42,"backend refused\n"};count=calls.size();
    const auto failed=uke::desktop::plasma_set("rotation","off");
    assert(failed.status==42&&failed.output==mutation.output&&calls.size()==count+2);mutation={0,""};
    for (const auto& output : {panel + panel, panel + response.output, std::string("Output: 2 DP-1\nAuto Rotate Policy: always\n"),
            std::string("Output: 1 DSI-1\nAuto Rotate Policy: incapable\nAutomatic brightness: unsupported\n"),
            panel + "Auto Rotate Policy: never\nAutomatic brightness: unsupported\n"}) {
        response.output = output;
        assert(uke::desktop::plasma_status().find("\"rotation\":null,\"automatic_brightness\":null") != std::string::npos);
        const auto count = calls.size();
        try { uke::desktop::plasma_set("rotation", "on"); assert(false); } catch (const std::runtime_error&) {}
        assert(calls.size() == count + 1);
    }
    response = {1, panel};
    assert(uke::desktop::plasma_status().find("\"rotation\":null") != std::string::npos);
    count = calls.size();
    try { uke::desktop::plasma_set("rotation", "on;touch /tmp/unsafe"); assert(false); } catch (const std::runtime_error&) {}
    assert(calls.size() == count);
}
