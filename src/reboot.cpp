// SPDX-License-Identifier: MIT
#include "boot-common.hpp"
#include <systemd/sd-bus.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

static const char *reboot_mode(const std::string &argument) {
    if(argument=="recovery" || argument=="recovey") return "recovery";
    if(argument=="bootloader") return "bootloader";
    return nullptr;
}
static bool reboot_backend(
    const std::filesystem::path &base="/sys/firmware/devicetree/base/soc/reboot_reason",
    const std::filesystem::path &driver="/sys/bus/platform/drivers/nvmem-reboot-mode") {
    if(read_text(base/"mode-recovery")!=std::string("\0\0\0\1",4) ||
       read_text(base/"mode-bootloader")!=std::string("\0\0\0\2",4)) return false;
    std::error_code error;
    for(const auto &entry:std::filesystem::directory_iterator(driver,error)) {
        if(std::filesystem::equivalent(entry.path()/"of_node",base,error) && !error) return true;
        error.clear();
    }
    return false;
}
static int request_reboot(sd_bus *bus,const char *mode) {
    constexpr auto service="org.freedesktop.login1";
    constexpr auto path="/org/freedesktop/login1";
    constexpr auto interface="org.freedesktop.login1.Manager";
    sd_bus_set_method_call_timeout(bus,5*1000*1000);
    char *previous=nullptr;
    int result=sd_bus_get_property_string(bus,service,path,interface,"RebootParameter",nullptr,&previous);
    if(result<0) return result;
    result=sd_bus_call_method(bus,service,path,interface,"SetRebootParameter",nullptr,nullptr,"s",mode);
    if(result>=0) {
        result=sd_bus_call_method(bus,service,path,interface,"Reboot",nullptr,nullptr,"b",0);
        // A refused shutdown must not change the target of a later ordinary reboot.
        if(result<0 && sd_bus_call_method(bus,service,path,interface,"SetRebootParameter",nullptr,nullptr,"s",previous)<0)
            std::cerr<<"Could not restore the previous reboot parameter\n";
    }
    free(previous);
    return result;
}
#ifndef UKE_REBOOT_TEST
int main(int argc,char **argv) {
    const char *mode=argc==2 ? reboot_mode(argv[1]) : nullptr;
    if(!mode) {
        // Preserve Fedora's ordinary reboot options without replacing its packaged binary.
        argv[0]=const_cast<char *>("reboot");
        execv("/usr/bin/systemctl",argv);
        std::cerr<<"Fedora reboot unavailable: "<<std::strerror(errno)<<'\n';
        return 1;
    }
    if(geteuid()!=0) {std::cerr<<"Use sudo reboot "<<mode<<'\n'; return 77;}
    if(!uke_device(read_text("/sys/firmware/devicetree/base/compatible")) || !reboot_backend()) {
        std::cerr<<"An exact Uke device and kernel reboot-mode backend are required\n"; return 2;
    }
    sd_bus *bus=nullptr;
    int result=sd_bus_open_system(&bus);
    if(result>=0) result=request_reboot(bus,mode);
    sd_bus_unref(bus);
    if(result<0) {std::cerr<<"Reboot request refused: "<<std::strerror(-result)<<'\n'; return 1;}
    std::cout<<"Reboot to "<<mode<<" requested\n";
    return 0;
}
#endif
