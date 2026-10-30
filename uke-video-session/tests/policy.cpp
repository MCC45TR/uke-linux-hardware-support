// SPDX-License-Identifier: MIT
#include "policy.hpp"
#include <cassert>
#include <map>

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
    assert(!CPU_ISSET(0, &selected));
    capacity[2] = 0;
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_EQUAL(&selected, &allowed));
    capacity.fill(800);
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_EQUAL(&selected, &allowed));
    CPU_ZERO(&allowed);
    CPU_SET(1, &allowed);
    CPU_SET(4, &allowed);
    capacity[1] = 385;
    selected = uke::preferred_cpus(allowed, capacity);
    assert(CPU_COUNT(&selected) == 1 && CPU_ISSET(4, &selected));
    std::map<pid_t, pid_t> parents{{20, 10}, {30, 20}, {40, 50}, {50, 40}, {60, 60}};
    const auto parent = [&](pid_t pid) { const auto i = parents.find(pid); return i == parents.end() ? 1 : i->second; };
    assert(uke::descendant(30, 10, parent));
    assert(!uke::descendant(30, 11, parent));
    assert(!uke::descendant(40, 10, parent));
    assert(!uke::descendant(60, 10, parent));
    assert(uke::youtube_url("https://www.youtube.com/watch?v=test"));
    assert(uke::youtube_url("https://youtu.be/test"));
    assert(!uke::youtube_url("file:///tmp/test"));
    assert(!uke::youtube_url("https://www.youtube.com.evil/watch"));
    assert(!uke::youtube_url("https://www.youtube.com/\n--no-sandbox"));
}
