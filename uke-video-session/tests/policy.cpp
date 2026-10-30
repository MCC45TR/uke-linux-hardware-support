// SPDX-License-Identifier: MIT
#include "policy.hpp"
#include <cassert>
int main() {
    const char argv[] = "/opt/google/chrome/chrome\0--type=gpu-process\0--ozone-platform=wayland";
    assert(uke::chrome_gpu_title(std::string_view(argv, sizeof(argv) - 1)));
    assert(uke::chrome_gpu_title("/opt/google/chrome/chrome --type=gpu-process --ozone-platform=wayland"));
    assert(!uke::chrome_gpu_title("/opt/google/chrome/chrome --type=renderer --url=--type=gpu-process"));
    assert(!uke::chrome_gpu_title("/tmp/chrome --type=gpu-process"));
    assert(!uke::chrome_gpu_title("/opt/google/chrome/chrome --type=gpu-process-fake"));
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    for (int cpu = 0; cpu < 8; ++cpu) CPU_SET(cpu, &allowed);
    std::array<unsigned, CPU_SETSIZE> capacity{};
    capacity.fill(385);
    for (int cpu = 3; cpu < 7; ++cpu) capacity[cpu] = 874;
    capacity[7] = 1024;
    auto selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_COUNT(&selected) == 5 && CPU_ISSET(3, &selected) && CPU_ISSET(7, &selected));
    capacity[2] = 0;
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_EQUAL(&selected, &allowed));
    capacity.fill(800);
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_EQUAL(&selected, &allowed));
    CPU_ZERO(&allowed); CPU_SET(1, &allowed); CPU_SET(4, &allowed);
    capacity[1] = 385;
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_COUNT(&selected) == 1 && CPU_ISSET(4, &selected));
    uke::ActivitySet streams;
    assert(streams.report(":1.1", 1, 20, 3840, 2160, true, 0));
    assert(streams.report(":1.2", 1, 21, 2160, 3840, true, 100));
    assert(streams.size() == 2 && streams.processes().size() == 2);
    streams.remove_sender(":1.1"); assert(streams.size() == 1);
    streams.expire(uke::video::expiry_usec); assert(streams.size() == 1);
    streams.expire(uke::video::expiry_usec + 100); assert(streams.size() == 0);
    assert(streams.report(":1.3", 2, 30, 1920, 1080, true, 200)); assert(streams.size() == 0);
    assert(!streams.report(":1.3", 0, 30, 3840, 2160, true, 200));
    for (unsigned i = 1; i <= 128; ++i) assert(streams.report(":1.4", i, 31, 3840, 2160, true, 200));
    assert(!streams.report(":1.4", 129, 31, 3840, 2160, true, 200));
    assert(streams.report(":1.4", 1, 31, 3840, 2160, false, 200)); assert(streams.size() == 127);
}
