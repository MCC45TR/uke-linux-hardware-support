// SPDX-License-Identifier: MIT
#include "affinity.hpp"
#include "policy.hpp"
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace uke {
namespace {
pid_t number(const std::string &name) {
    pid_t pid = -1;
    const auto [end, error] = std::from_chars(name.data(), name.data() + name.size(), pid);
    return error == std::errc{} && end == name.data() + name.size() && pid > 1 ? pid : -1;
}
std::string mask_string(const cpu_set_t &mask) {
    std::string text;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        if (CPU_ISSET(cpu, &mask)) text += (text.empty() ? "" : ",") + std::to_string(cpu);
    return text;
}
bool read_mask(const std::string &text, cpu_set_t &mask) {
    CPU_ZERO(&mask);
    std::istringstream parts(text);
    std::string part;
    while (std::getline(parts, part, ',')) {
        unsigned cpu = CPU_SETSIZE;
        const auto [end, error] = std::from_chars(part.data(), part.data() + part.size(), cpu);
        if (error != std::errc{} || end != part.data() + part.size() || cpu >= CPU_SETSIZE) return false;
        CPU_SET(cpu, &mask);
    }
    return CPU_COUNT(&mask);
}
std::uint64_t ticks_now() {
    timespec now{};
    clock_gettime(CLOCK_BOOTTIME, &now);
    const auto hz = sysconf(_SC_CLK_TCK);
    return std::uint64_t(now.tv_sec) * hz + std::uint64_t(now.tv_nsec) * hz / 1000000000;
}
bool owned(pid_t pid) {
    std::ifstream status("/proc/" + std::to_string(pid) + "/status");
    std::string key;
    while (status >> key) {
        if (key == "Uid:") {
            uid_t real = ~uid_t{}, effective = ~uid_t{};
            status >> real >> effective;
            return status && effective == geteuid();
        }
        std::string rest;
        std::getline(status, rest);
    }
    return false;
}
std::set<pid_t> threads(pid_t pid) {
    std::set<pid_t> result;
    std::error_code error;
    for (fs::directory_iterator i("/proc/" + std::to_string(pid) + "/task", error), end;
         !error && i != end; i.increment(error)) {
        const auto tid = number(i->path().filename().string());
        if (tid > 1) result.insert(tid);
    }
    return result;
}
}
std::uint64_t process_start(pid_t pid, pid_t tid) {
    const auto base = "/proc/" + std::to_string(pid) + (tid ? "/task/" + std::to_string(tid) : "");
    std::ifstream file(base + "/stat");
    std::string line, field;
    std::getline(file, line);
    const auto close = line.rfind(')');
    if (close == std::string::npos) return 0;
    std::istringstream fields(line.substr(close + 1));
    for (int i = 3; i < 22; ++i) if (!(fields >> field)) return 0;
    std::uint64_t start = 0;
    fields >> start;
    return start;
}
bool chrome_gpu(pid_t pid) {
    if (!owned(pid)) return false;
    const auto base = "/proc/" + std::to_string(pid);
    std::error_code error;
    const auto executable = fs::read_symlink(base + "/exe", error).string();
    if (error || (executable != "/opt/google/chrome/chrome" &&
                  executable != "/opt/google/chrome/chrome (deleted)")) return false;
    std::ifstream command(base + "/cmdline");
    char title[512]{};
    command.read(title, sizeof(title));
    return chrome_gpu_title(std::string_view(title, command.gcount()));
}
AffinityHints::AffinityHints(std::string directory, cpu_set_t preferred,
                             std::function<bool(pid_t)> eligible)
    : directory_(std::move(directory)), eligible_(std::move(eligible)), preferred_(preferred) {
    if (mkdir(directory_.c_str(), 0700) && errno != EEXIST) healthy_ = false;
    struct stat info{};
    if (lstat(directory_.c_str(), &info) || !S_ISDIR(info.st_mode) ||
        info.st_uid != geteuid() || (info.st_mode & 077)) healthy_ = false;
}
AffinityHints::~AffinityHints() { update({}); }
bool AffinityHints::save() const {
    if (!healthy_) return false;
    std::ostringstream data;
    data << "UKE_VIDEO_AFFINITY1\n";
    for (const auto &[pid, process] : processes_) {
        data << "P " << pid << ' ' << process.start << ' ' << process.activation << ' '
             << mask_string(process.base) << ' ' << mask_string(process.preferred) << '\n';
        for (const auto &[tid, thread] : process.threads)
            data << "T " << pid << ' ' << tid << ' ' << thread.start << ' '
                 << mask_string(thread.original) << ' ' << mask_string(thread.applied) << ' '
                 << thread.managed << '\n';
    }
    const auto text = data.str();
    if (text.size() > 1024 * 1024) return false;
    int dir = open(directory_.c_str(), O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) return false;
    int fd = openat(dir, "affinity.new", O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0600);
    bool ok = fd >= 0;
    struct stat info{};
    if (ok) ok = !fstat(fd, &info) && S_ISREG(info.st_mode) && info.st_uid == geteuid() && !fchmod(fd, 0600);
    std::size_t offset = 0;
    while (ok && offset < text.size()) {
        const auto written = write(fd, text.data() + offset, text.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) { ok = false; break; }
        offset += written;
    }
    if (ok) ok = !fsync(fd);
    if (fd >= 0) close(fd);
    if (ok) ok = !renameat(dir, "affinity.new", dir, "affinity.state");
    if (!ok) unlinkat(dir, "affinity.new", 0);
    close(dir);
    return ok;
}
bool AffinityHints::load() {
    if (!healthy_) return false;
    const auto path = directory_ + "/affinity.state";
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return errno == ENOENT;
    struct stat info{};
    if (fstat(fd, &info) || info.st_uid != geteuid() || !S_ISREG(info.st_mode) ||
        (info.st_mode & 077) || info.st_size > 1024 * 1024) { close(fd); return false; }
    std::string data(info.st_size, '\0');
    std::size_t offset = 0;
    while (offset < data.size()) {
        const auto count = read(fd, data.data() + offset, data.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { close(fd); return false; }
        offset += count;
    }
    close(fd);
    std::istringstream file(data);
    std::string line;
    if (!std::getline(file, line) || line != "UKE_VIDEO_AFFINITY1") return false;
    std::map<pid_t, Process> parsed;
    unsigned count = 0;
    while (std::getline(file, line)) {
        if (++count > 1024) return false;
        std::istringstream row(line);
        char type = 0;
        pid_t pid = 0;
        std::string original, applied, extra;
        row >> type >> pid;
        if (type == 'P') {
            Process p{};
            if (!(row >> p.start >> p.activation >> original >> applied) || parsed.count(pid) ||
                pid <= 1 || !read_mask(original, p.base) || !read_mask(applied, p.preferred)) return false;
            parsed[pid] = p;
        } else if (type == 'T') {
            pid_t tid = 0;
            Thread t{};
            if (!(row >> tid >> t.start >> original >> applied >> t.managed) ||
                !parsed.count(pid) || tid <= 1 || parsed[pid].threads.count(tid) ||
                !read_mask(original, t.original) || !read_mask(applied, t.applied)) return false;
            t.changed = true;
            parsed[pid].threads[tid] = t;
        } else return false;
        if (row >> extra) return false;
    }
    processes_ = std::move(parsed);
    return true;
}
bool AffinityHints::restore(pid_t pid, Process &process) {
    if (!owned(pid) || process_start(pid) != process.start) return true;
    bool ok = true;
    for (const auto tid : threads(pid)) {
        cpu_set_t current;
        const auto start = process_start(pid, tid);
        if (!start || sched_getaffinity(tid, sizeof(current), &current)) continue;
        const auto i = process.threads.find(tid);
        const cpu_set_t *original = nullptr, *applied = nullptr;
        if (i != process.threads.end()) {
            if (i->second.start != start || !i->second.managed) continue;
            original = &i->second.original;
            applied = &i->second.applied;
        } else if (start >= process.activation) {
            // Newly created GPU threads may have inherited our process-main hint.
            original = &process.base;
            applied = &process.preferred;
        }
        if (original && CPU_EQUAL(&current, applied) &&
            sched_setaffinity(tid, sizeof(*original), original) && errno != ESRCH) ok = false;
    }
    return ok;
}
bool AffinityHints::recover() {
    if (!load()) { healthy_ = false; return false; }
    return update({});
}
bool AffinityHints::update(const std::set<pid_t> &wanted) {
    if (!healthy_) return false;
    bool changed = false;
    for (auto i = processes_.begin(); i != processes_.end();) {
        if (!wanted.count(i->first) || !eligible_(i->first) || process_start(i->first) != i->second.start) {
            if (restore(i->first, i->second)) { i = processes_.erase(i); changed = true; }
            else ++i;
        } else ++i;
    }
    for (const auto pid : wanted) {
        if (!eligible_(pid)) continue;
        if (!processes_.count(pid)) {
            Process p{};
            p.start = process_start(pid);
            p.activation = ticks_now();
            if (!p.start || sched_getaffinity(pid, sizeof(p.base), &p.base)) continue;
            CPU_AND(&p.preferred, &p.base, &preferred_);
            if (!CPU_COUNT(&p.preferred)) continue;
            processes_[pid] = p;
            changed = true;
        }
        auto &p = processes_.at(pid);
        const auto live = threads(pid);
        for (auto i = p.threads.begin(); i != p.threads.end();)
            if (!live.count(i->first)) { i = p.threads.erase(i); changed = true; } else ++i;
        for (const auto tid : live) {
            cpu_set_t current;
            const auto start = process_start(pid, tid);
            if (!start || sched_getaffinity(tid, sizeof(current), &current)) continue;
            auto i = p.threads.find(tid);
            if (i != p.threads.end() && i->second.start != start) { p.threads.erase(i); i = p.threads.end(); }
            if (i == p.threads.end()) {
                Thread t{};
                t.start = start;
                t.original = current;
                if (start >= p.activation && CPU_EQUAL(&current, &p.preferred)) t.original = p.base;
                CPU_AND(&t.applied, &current, &p.preferred);
                if (!CPU_COUNT(&t.applied)) continue;
                p.threads[tid] = t;
                changed = true;
            } else if (i->second.changed && i->second.managed && !CPU_EQUAL(&current, &i->second.applied)) {
                i->second.managed = false; // Respect a later explicit affinity change.
                changed = true;
            }
        }
    }
    if (changed && !save()) return false; // Never apply a hint without its recovery record.
    for (auto &[pid, p] : processes_)
        if (wanted.count(pid)) for (auto &[tid, t] : p.threads)
            if (t.managed && !t.changed && process_start(pid, tid) == t.start) {
                if (!sched_setaffinity(tid, sizeof(t.applied), &t.applied)) t.changed = true;
            }
    return true;
}
}
