// SPDX-License-Identifier: MIT
#include "hub.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
int main(){char tmp[]="/tmp/uke-hub-XXXXXX";auto path=mkdtemp(tmp);assert(path);fs::path root=path;
    auto empty=uke::hub::snapshot(root);assert(empty.find("\"state\":\"unknown\"")!=std::string::npos && empty.find("\"state\":\"unsupported\"")!=std::string::npos);
    put(root/"proc/sys/kernel/osrelease","fixture-kernel");put(root/"proc/meminfo","MemTotal: 8192 kB\nMemAvailable: 4096 kB\n");
    auto battery=root/"sys/class/power_supply/test";put(battery/"type","Battery");put(battery/"capacity","73");put(battery/"status","Discharging");
    auto zone=root/"sys/class/thermal/thermal_zone0";put(zone/"type","fixture");put(zone/"temp","41000");
    auto data=uke::hub::snapshot(root);assert(data.find("\"value\":\"73\"")!=std::string::npos && data.find("\"value\":\"4 / 8\"")!=std::string::npos);
    put(battery/"capacity","101");assert(uke::hub::snapshot(root).find("invalid numeric sample")!=std::string::npos);
    put(battery/"type","");data=uke::hub::snapshot(root);assert(data.find("\"id\":\"battery\",\"label\":\"Battery\",\"state\":\"unknown\"")!=std::string::npos);
    fs::remove(zone/"temp");fs::create_symlink("/etc/passwd",zone/"temp");assert(uke::hub::snapshot(root).find("uncontained interface")!=std::string::npos);
    fs::remove_all(root);std::cout<<"Hardware hub: live values, unknown/unsupported/error states, numeric bounds and containment passed with fixtures\n";
}
