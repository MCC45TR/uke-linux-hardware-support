// SPDX-License-Identifier: MIT
#include "../src/state.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace uke_pogo_desktop;
static void text_file(const fs::path &p,const std::string &v) {std::ofstream f(p);f<<v;f.close();assert(f.good());}
int main() {
    char pattern[]="/tmp/uke-pogo-desktop-XXXXXX";auto made=mkdtemp(pattern);assert(made);
    const fs::path root=made,sys=root/"sys",client=sys/"devices/platform/pogo",led=client/"leds"/led_name;
    for(const auto &p:{sys/of_path,sys/"bus/i2c/drivers/hid-nanosic-uke",sys/"class/leds",led})fs::create_directories(p);
    fs::create_directory_symlink(client,led/"device");
    fs::create_directory_symlink(sys/of_path,client/"of_node");
    fs::create_directory_symlink(sys/"bus/i2c/drivers/hid-nanosic-uke",client/"driver");
    const auto read=[](const auto &path){std::ifstream f(path);return std::string(std::istreambuf_iterator<char>(f),{});};
    const auto detected=[&]{return present(sys,read);};
    assert(!detected());fs::create_directory_symlink(led,sys/"class/leds"/led_name);
    text_file(led/"max_brightness","100\n");assert(!detected());
    text_file(client/"bridge_health","attached=1\nframes=0\n");assert(detected());
    text_file(client/"bridge_health","attached=0\nframes=0\n");assert(!detected());
    text_file(client/"bridge_health","attached=1\nframes=0\n");
    text_file(led/"max_brightness","100");assert(!detected());
    text_file(led/"max_brightness","255\n");assert(!detected());
    text_file(led/"max_brightness","100\n");
    fs::remove(client/"driver");assert(!detected());
    fs::create_directory_symlink(sys,client/"driver");assert(!detected());
    fs::remove(client/"driver");fs::create_directory_symlink(sys/"bus/i2c/drivers/hid-nanosic-uke",client/"driver");
    fs::remove(client/"of_node");fs::create_directory_symlink(sys,client/"of_node");assert(!detected());
    fs::remove(client/"of_node");fs::create_directory_symlink(sys/of_path,client/"of_node");assert(detected());
    fs::remove(sys/"class/leds"/led_name);assert(!detected());
    fs::remove_all(root);
    const std::string board("xiaomi,uke\0qcom,sm7675\0",sizeof("xiaomi,uke\0qcom,sm7675\0")-1);
    assert(compatible(board,"xiaomi,uke")&&compatible(board,"qcom,sm7675"));
    assert(!compatible("xiaomi,uke","xiaomi,uke"));assert(!compatible(board,"xiaomi,foreign"));
    Refresh policy;Observation s;
    s.upower_ready=s.powerdevil_running=true;assert(!policy.decide(s));
    s.pogo=true;s.upower_ready=false;assert(!policy.decide(s));
    s.upower_ready=true;s.upower_max=100;assert(policy.decide(s));assert(!policy.decide(s));
    s.powerdevil_supported=true;s.powerdevil_max=100;assert(!policy.decide(s));
    s.pogo=false;s.upower_ready=false;assert(!policy.decide(s));
    s.upower_ready=true;s.upower_max=0;assert(policy.decide(s));assert(!policy.decide(s));
    s.pogo=true;s.powerdevil_supported=false;s.powerdevil_max=0;assert(!policy.decide(s));
    s.upower_max=100;assert(policy.decide(s));
    s.pogo=false;s.upower_max=s.powerdevil_max=50;s.powerdevil_supported=true;
    assert(!policy.decide(s)); // A different backlit keyboard remains supported.
    s.powerdevil_supported=false;assert(!policy.decide(s)); // No unrelated refresh after pogo removal settled.
    Refresh down; s={true,true,false,false,100,0};assert(!down.decide(s));
    std::cout<<"PASS: actual filesystem identity and hotplug policy; UPower readiness, stale KDE cache, one refresh per edge, other keyboard preservation\n";
}
