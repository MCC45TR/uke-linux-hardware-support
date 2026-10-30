// SPDX-License-Identifier: MIT
#pragma once
#include "protocol.hpp"
#include <array>
#include <map>
#include <sched.h>
#include <set>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace uke {
inline bool chrome_gpu_title(std::string_view title) {
    const auto token = [&title]() {
        while (!title.empty() && (title.front() == '\0' || title.front() == ' ')) title.remove_prefix(1);
        std::size_t end = 0;
        while (end < title.size() && title[end] != '\0' && title[end] != ' ') ++end;
        const auto result = title.substr(0, end);
        title.remove_prefix(end);
        return result;
    };
    return token() == "/opt/google/chrome/chrome" && token() == "--type=gpu-process";
}
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
        if (CPU_ISSET(cpu, &allowed) && capacity[cpu] > minimum) CPU_SET(cpu, &preferred);
    return CPU_COUNT(&preferred) ? preferred : allowed;
}
class ActivitySet {
    struct Entry { pid_t pid; std::uint64_t seen; };
    std::map<std::pair<std::string, std::uint64_t>, Entry> entries_;
public:
    bool report(const std::string &sender, std::uint64_t stream, pid_t pid,
                unsigned width, unsigned height, bool active, std::uint64_t now) {
        const auto key = std::make_pair(sender, stream);
        if (!active || std::uint64_t(width) * height < video::uhd_pixels) {
            entries_.erase(key);
            return true;
        }
        if (!stream || pid <= 1 || (!entries_.count(key) && entries_.size() >= 128)) return false;
        entries_[key] = {pid, now};
        return true;
    }
    void remove_sender(const std::string &sender) {
        for (auto i = entries_.begin(); i != entries_.end();)
            if (i->first.first == sender) i = entries_.erase(i); else ++i;
    }
    void expire(std::uint64_t now) {
        for (auto i = entries_.begin(); i != entries_.end();)
            if (now - i->second.seen >= video::expiry_usec) i = entries_.erase(i); else ++i;
    }
    unsigned size() const { return entries_.size(); }
    std::set<pid_t> processes() const {
        std::set<pid_t> result;
        for (const auto &entry : entries_) result.insert(entry.second.pid);
        return result;
    }
};
}
