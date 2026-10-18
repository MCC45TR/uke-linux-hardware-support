// SPDX-License-Identifier: MIT
#include "../src/offline/boot-mode.hpp"
#include "../src/offline/screen.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace uke::offline;
int main(int argc,char **argv) {
    for (const auto &s : {"", "root=PARTLABEL=linux systemd.unit=multi-user.target", "foo=androidboot.mode=charger", "androidboot.mode=normal", "-- androidboot.mode=charger"}) {
        BootSignal b; b.command_line(s); assert(b.mode()==BootMode::normal);
    }
    BootSignal b; b.command_line("root=PARTLABEL=linux androidboot.mode=charger"); assert(b.mode()==BootMode::charger);
    b.bootconfig("androidboot.mode = \"charger\"\nandroidboot.serialno = \"private\"\n"); assert(b.mode()==BootMode::charger);
    b.command_line("androidboot.mode=normal"); assert(b.mode()==BootMode::invalid);
    for(const auto &s : {"androidboot.mode=charger;reboot", "androidboot.mode=", "androidboot.mode=CHARGER!"}) {
        BootSignal bad; bad.command_line(s); assert(bad.mode()==BootMode::invalid);
    }
    for(const auto &s : {"androidboot.mode = \"charger\"\n", " androidboot.mode = charger\n"}) {
        BootSignal config; config.bootconfig(s); assert(config.mode()==BootMode::charger);
    }
    BootSignal oversized; oversized.command_line(std::string(16385,'x')); assert(oversized.mode()==BootMode::invalid);
    BootSignal config_limit; config_limit.bootconfig(std::string(65537,'x')); assert(config_limit.mode()==BootMode::invalid);
    assert(!compatible("xiaomi,uke2")); assert(!compatible("qcom,sm7675"));
    const std::string dt("xiaomi,uke\0qcom,sm7675\0",23); assert(compatible(dt));
    Screen screen; Snapshot s; screen.render(s); const auto unknown=screen.pixels;
    s.valid=true; s.input_known=true; s.online=true; s.percent=80; s.status="Not charging"; s.health="Good";
    s.power_uw=8250000; s.temperature_decic=270;
    screen.render(s); assert(screen.pixels!=unknown);
    assert(screen.pixels.size()==Screen::width*Screen::height);
    if(argc==2) {
        std::ofstream image(argv[1],std::ios::binary);
        image<<"P6\n720 480\n255\n";
        for(auto p:screen.pixels) { const char rgb[3]={static_cast<char>(p>>16),static_cast<char>(p>>8),static_cast<char>(p)}; image.write(rgb,3); }
        assert(image.good());
    }
    std::cout<<"Offline boot signals, conflicting/malformed input, identity and honest UI state passed; no hardware access.\n";
}
