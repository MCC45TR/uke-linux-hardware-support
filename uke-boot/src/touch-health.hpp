// SPDX-License-Identifier: MIT
#pragma once
#include "boot-common.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace uke_touch {
namespace fs=std::filesystem;
static inline bool compatible(const std::string &value,const std::string &expected) {
    bool found=false;
    for(std::size_t start=0;start<value.size();) {
        const auto end=value.find('\0',start);
        if(end==std::string::npos)return false;
        found|=value.compare(start,end-start,expected)==0;start=end+1;
    }
    return found;
}
static inline bool native_healthy(const fs::path &root) {
    fs::path node; unsigned native=0,total=0;
    for(const auto &entry:fs::directory_iterator(root/"sys/bus/spi/devices")) {
        const auto value=read_text(entry.path()/"of_node/compatible");
        const bool input=compatible(value,"novatek,nt36532-uke-touch");
        native+=input;
        total+=input+compatible(value,"novatek,NVT-ts-spi")+compatible(value,"novatek,nt36532-uke-thp");
        if(input)node=entry.path();
    }
    if(!native)return false;
    if(native!=1 || total!=1 || !uke_device(read_text(root/"sys/firmware/devicetree/base/compatible")))
        throw std::runtime_error("Ambiguous or foreign native touchscreen");
    if(!fs::is_symlink(node/"driver") || fs::canonical(node/"driver")!=
       fs::canonical(root/"sys/bus/spi/drivers/nt36532-uke-touch"))
        throw std::runtime_error("Native touchscreen driver is not bound");
    unsigned inputs=0;
    for(const auto &entry:fs::directory_iterator(node/"input")) {
        if(read_text(entry.path()/"name").empty())
            throw std::runtime_error("Native touchscreen name is missing");
        auto bits=read_text(entry.path()/"capabilities/abs");
        if(!bits.empty() && bits.back()=='\n')bits.pop_back();
        if(bits.empty() || bits.size()>16 || bits.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
            throw std::runtime_error("Native touchscreen capabilities are malformed");
        constexpr auto required=(1ULL<<47)|(1ULL<<53)|(1ULL<<54)|(1ULL<<57);
        if((std::stoull(bits,nullptr,16)&required)!=required || read_text(entry.path()/"properties")!="2\n")
            throw std::runtime_error("Native touchscreen multitouch capabilities are missing");
        ++inputs;
    }
    if(inputs!=1)
        throw std::runtime_error("Native touchscreen input is ambiguous");
    const auto stats=read_text(node/"input_stats");
    if(stats.ends_with(" ready=1\n"))return true;
    unsigned panels=0;bool disabled=false;
    // Disabling the internal panel intentionally quiesces native touch firmware.
    if(stats.ends_with(" ready=0\n") && fs::is_directory(root/"sys/class/drm")) {
        for(const auto &entry:fs::directory_iterator(root/"sys/class/drm")) {
            if(!entry.path().filename().string().ends_with("-DSI-1"))continue;
            ++panels;
            disabled=read_text(entry.path()/"status")=="connected\n" &&
                     read_text(entry.path()/"enabled")=="disabled\n";
        }
    }
    if(panels!=1 || !disabled)
        throw std::runtime_error("Native touchscreen input or firmware is not ready");
    return true;
}
static inline std::vector<std::string> health_units(const fs::path &root) {
    std::vector<std::string> units{"display-manager.service","uke-touchscreen.service",
        "uke-adaptive-display.target","iio-sensor-proxy.service","NetworkManager.service"};
    if(native_healthy(root))std::erase(units,"uke-touchscreen.service");
    return units;
}
}
