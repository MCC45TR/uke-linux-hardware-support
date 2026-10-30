// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <sched.h>
#include <set>
#include <string>
#include <sys/types.h>

namespace uke {
std::uint64_t process_start(pid_t pid, pid_t tid = 0);
bool chrome_gpu(pid_t pid);
class AffinityHints {
    struct Thread { std::uint64_t start; cpu_set_t original, applied; bool managed = true, changed = false; };
    struct Process {
        std::uint64_t start, activation;
        cpu_set_t base, preferred;
        std::map<pid_t, Thread> threads;
    };
    std::map<pid_t, Process> processes_;
    std::string directory_;
    std::function<bool(pid_t)> eligible_;
    cpu_set_t preferred_;
    bool healthy_ = true;
    bool save() const;
    bool load();
    bool restore(pid_t pid, Process &process);
public:
    AffinityHints(std::string directory, cpu_set_t preferred, std::function<bool(pid_t)> eligible);
    ~AffinityHints();
    bool recover();
    bool update(const std::set<pid_t> &wanted);
};
}
