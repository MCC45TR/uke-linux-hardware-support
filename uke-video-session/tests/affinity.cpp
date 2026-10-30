// SPDX-License-Identifier: MIT
#include "affinity.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
    cpu_set_t original, preferred, current;
    assert(!sched_getaffinity(0, sizeof(original), &original));
    if (CPU_COUNT(&original) < 2) { std::puts("Affinity control skipped: worker has one allowed CPU"); return 0; }
    CPU_ZERO(&preferred);
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) if (CPU_ISSET(cpu, &original)) { CPU_SET(cpu, &preferred); break; }
    char path[] = "/tmp/uke-affinity-check.XXXXXX";
    assert(mkdtemp(path));
    const auto child = fork();
    assert(child >= 0);
    if (!child) { for (;;) pause(); }
    const auto eligible = [child](pid_t pid) { return pid == child; };
    {
        uke::AffinityHints hints(path, preferred, eligible);
        assert(hints.recover()); assert(hints.update({child}));
        assert(!sched_getaffinity(child, sizeof(current), &current) && CPU_EQUAL(&current, &preferred));
        assert(hints.update({}));
        assert(!sched_getaffinity(child, sizeof(current), &current) && CPU_EQUAL(&current, &original));
        assert(hints.update({child}));
        assert(!sched_setaffinity(child, sizeof(original), &original));
        assert(hints.update({child})); // An explicit later affinity change wins.
        assert(hints.update({}));
        assert(!sched_getaffinity(child, sizeof(current), &current) && CPU_EQUAL(&current, &original));
    }
    const auto crashed_agent = fork(); assert(crashed_agent >= 0);
    if (!crashed_agent) {
        uke::AffinityHints hints(path, preferred, eligible);
        if (!hints.recover() || !hints.update({child})) _exit(1);
        _exit(0); // Leave the record and hint, like an ungraceful service exit.
    }
    int status = 0; assert(waitpid(crashed_agent, &status, 0) == crashed_agent && status == 0);
    assert(!sched_getaffinity(child, sizeof(current), &current) && CPU_EQUAL(&current, &preferred));
    {
        uke::AffinityHints hints(path, preferred, eligible);
        assert(hints.recover());
        assert(!sched_getaffinity(child, sizeof(current), &current) && CPU_EQUAL(&current, &original));
    }
    std::ofstream(std::string(path) + "/affinity.state") << "invalid journal\n";
    {
        uke::AffinityHints hints(path, preferred, eligible);
        assert(!hints.recover()); assert(!hints.update({child}));
    }
    kill(child, SIGTERM); assert(waitpid(child, &status, 0) == child);
    std::filesystem::remove_all(path);
    std::puts("Affinity apply, idle restore, manual change, crash recovery and malformed record checks passed");
}
