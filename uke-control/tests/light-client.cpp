// SPDX-License-Identifier: MIT
// Actual connection cache; simulated bus handles and process identity.
#include <atomic>
#include <cassert>
#include <thread>
#include <systemd/sd-bus.h>
#include <unistd.h>
namespace client_fixture {
static std::atomic<unsigned> opened{0},closed{0};
static thread_local pid_t process=100;
static bool failing=false;
static pid_t pid(){return process;}
static int open(sd_bus** out){
    if(failing){*out=nullptr;return -1;}
    *out=reinterpret_cast<sd_bus*>(uintptr_t(++opened));return 0;
}
static sd_bus* unref(sd_bus* bus){if(bus)++closed;return nullptr;}
}
#define sd_bus_open_system client_fixture::open
#define sd_bus_unref client_fixture::unref
#define getpid client_fixture::pid
#include "../src/lights.cpp"
#undef sd_bus_open_system
#undef sd_bus_unref
#undef getpid
int main() {
    using namespace client_fixture;
    const auto first=uke::lights::client_bus();assert(first && opened==1 && !closed);
    assert(uke::lights::client_bus()==first && opened==1);
    process=101;const auto second=uke::lights::client_bus();assert(second!=first && opened==2 && closed==1);
    std::thread worker([&]{assert(uke::lights::client_bus()!=second && opened==3);});worker.join();
    assert(closed==2 && uke::lights::client_bus()==second);
    process=102;failing=true;bool refused=false;
    try{uke::lights::client_bus();}catch(...){refused=true;}
    assert(refused && opened==3 && closed==3);
    failing=false;assert(uke::lights::client_bus() && opened==4 && closed==3);
}
