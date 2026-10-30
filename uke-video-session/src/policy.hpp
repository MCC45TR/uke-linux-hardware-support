// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <sched.h>
#include <string_view>
#include <sys/types.h>

namespace uke {
inline cpu_set_t preferred_cpus(const cpu_set_t &allowed,
                               const std::array<unsigned, CPU_SETSIZE> &capacity) {
    unsigned minimum = ~0u;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (!CPU_ISSET(cpu, &allowed)) continue;
        if (!capacity[cpu]) return allowed;
        if (capacity[cpu] < minimum) minimum = capacity[cpu];
    }
    cpu_set_t preferred;
    CPU_ZERO(&preferred);
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        if (CPU_ISSET(cpu, &allowed) && capacity[cpu] > minimum)
            CPU_SET(cpu, &preferred);
    return CPU_COUNT(&preferred) ? preferred : allowed;
}

template<class Parent> bool descendant(pid_t pid, pid_t root, Parent parent) {
    // A bounded walk also rejects cycles and incomplete /proc snapshots.
    for (unsigned depth = 0; depth < 128 && pid > 1; ++depth) {
        if (pid == root) return true;
        const auto next = parent(pid);
        if (next == pid) return false;
        pid = next;
    }
    return false;
}

inline bool youtube_url(std::string_view url) {
    return (url.starts_with("https://www.youtube.com/") ||
            url.starts_with("https://youtu.be/")) &&
           url.find_first_of("\r\n\t ") == std::string_view::npos;
}
} // namespace uke
