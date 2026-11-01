// SPDX-License-Identifier: MIT
#include "apt-policy.hpp"
#include <cassert>
#include <functional>
#include <iostream>
using namespace uke::apt;
static void rejects(const std::function<void()>& action) {
    bool rejected=false;
    try { action(); } catch(const std::runtime_error&) { rejected=true; }
    assert(rejected);
}
int main() {
    const std::string source="Types: deb\nURIs: https://ppa.launchpadcontent.net/mcs45tr/uke-linux-unstable/ubuntu/\nSuites: tenacious\nComponents: main\nArchitectures: arm64\nSigned-By: /usr/share/keyrings/uke-linux-archive.gpg\n";
    assert(inspect_managed_sources(source)=="unstable");
    assert(inspect_managed_sources(source+"Enabled: no\n").empty());
    assert(inspect_managed_sources("").empty());
    rejects([&]{ inspect_managed_sources(source+"\n"+source); });
    rejects([&]{ inspect_managed_sources(source+"Trusted: yes\n"); });
    rejects([&]{ inspect_managed_sources(source+"Allow-Insecure: yes\n"); });
    rejects([&]{ inspect_managed_sources(source+"suites: tenacious\n"); });
    for(const auto& [from,to]:std::vector<std::pair<std::string,std::string>>{
        {"unstable","stable"},{"mcs45tr","MCC45TR"},{"tenacious","tenacious noble"},
        {"arm64","arm64 amd64"},{"deb\n","deb deb-src\n"},{"https:","http:"},
        {"Signed-By:","X-Signed-By:"},{"/usr/share/keyrings/uke-linux-archive.gpg","/tmp/key.gpg"}}) {
        auto bad=source; bad.replace(bad.find(from),from.size(),to);
        rejects([&]{ inspect_managed_sources(bad); });
    }
    rejects([]{ sources(" orphan\n"); });
    rejects([]{ sources(std::string("Types: deb\0",11)); });
    const auto inventory=installed("uke-control\tarm64\t2.8.0-1\thold\tok\tinstalled\nlibc6\tarm64\t2.42-1\tinstall\tok\tinstalled\n");
    assert(inventory.size()==2);
    assert(exclusions(inventory,{{"libc6","arm64","upgrade"}}).empty());
    assert(!exclusions(inventory,{{"uke-control","arm64","upgrade"}}).empty());
    for(const auto& name:{"linux-image-uke","uke-boot-integration","uke-dtb","uke-local-touch-data","uke-firmware","uke-orangefox-recovery"})
        assert(!exclusions(inventory,{{name,"arm64","upgrade"}}).empty());
    for(const auto& action:{"remove","purge","downgrade","reinstall","unknown"})
        assert(!exclusions(inventory,{{"libc6","arm64",action}}).empty());
    assert(!exclusions(inventory,{{"libc6","arm64","upgrade"},{"libc6","arm64","upgrade"}}).empty());
    assert(!exclusions(inventory,{{"libc6","amd64","upgrade"}}).empty());
    for(const auto& status:{"half-installed","unpacked","half-configured","triggers-awaited","triggers-pending"})
        rejects([&]{ installed(std::string("libc6\tarm64\t2.42-1\tinstall\tok\t")+status+"\n"); });
    rejects([]{ installed("libc6\tarm64\t2.42-1\tinstall\treinstreq\tinstalled\n"); });
    rejects([]{ installed("libc6\tamd64\t2.42-1\tinstall\tok\tinstalled\n"); });
    rejects([]{ installed("libc6\tarm64\t2.42-1\tinstall\tok\tinstalled\nlibc6\tarm64\t2.42-1\tinstall\tok\tinstalled\n"); });
    rejects([]{ installed("libc6\tarm64\t2.42-1\tinstall\tok\tinstalled\textra\n"); });
    std::cout<<"PASS: APT source configuration, dpkg state and preliminary exclusions; no transaction executed\n";
}
