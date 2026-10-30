// SPDX-License-Identifier: MIT
// Run under dbus-run-session. Both owned connections target that private bus.
#include "desktop.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
extern "C" int __wrap_sd_bus_open_system(sd_bus** bus) { return sd_bus_open_user(bus); }
static size_t descriptors() {
    size_t count=0;
    for([[maybe_unused]] const auto& entry:std::filesystem::directory_iterator("/proc/self/fd"))++count;
    return count;
}
int main() {
    for(unsigned i=0;i<5;++i)(void)uke::desktop::status();
    const auto before=descriptors();
    for(unsigned i=0;i<100;++i) {
        const auto snapshot=uke::desktop::status();
        assert(snapshot.find("\"service_activation\":false")!=snapshot.npos);
        assert(descriptors()==before);
    }
    std::cout<<"Desktop resources: 100 owned system/session connection cycles retain "<<before<<" descriptors; private bus only\n";
}
