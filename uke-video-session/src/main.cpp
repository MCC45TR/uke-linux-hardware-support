// SPDX-License-Identifier: MIT
#include "policy.hpp"
#include <systemd/sd-bus.h>
#include <charconv>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

namespace fs = std::filesystem;
namespace {
volatile sig_atomic_t received_signal = 0;
void signal_received(int signal) { received_signal = signal; }

pid_t numeric_pid(const std::string &name) {
    pid_t pid = -1;
    const auto [end, error] = std::from_chars(name.data(), name.data() + name.size(), pid);
    return error == std::errc{} && end == name.data() + name.size() && pid > 0 ? pid : -1;
}
pid_t parent_pid(pid_t pid) {
    std::ifstream status("/proc/" + std::to_string(pid) + "/status");
    std::string key;
    while (status >> key) {
        if (key == "PPid:") {
            pid_t parent = -1;
            status >> parent;
            return parent;
        }
        status.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return -1;
}
bool chrome_gpu(pid_t pid, pid_t browser) {
    const auto base = "/proc/" + std::to_string(pid);
    struct stat info{};
    if (stat(base.c_str(), &info) || info.st_uid != geteuid()) return false;
    std::error_code error;
    if (fs::read_symlink(base + "/exe", error) != "/opt/google/chrome/chrome" || error)
        return false;
    std::ifstream command(base + "/cmdline");
    std::string argument;
    bool gpu = false;
    while (std::getline(command, argument, '\0'))
        if (argument == "--type=gpu-process") gpu = true;
    return gpu && uke::descendant(pid, browser, parent_pid);
}
bool hint_gpu(pid_t browser, const cpu_set_t &preferred) {
    bool found = false;
    std::error_code error;
    for (fs::directory_iterator entry("/proc", error), end;
         !error && entry != end; entry.increment(error)) {
        const auto pid = numeric_pid(entry->path().filename().string());
        if (pid <= 0 || !chrome_gpu(pid, browser)) continue;
        found = true;
        std::error_code task_error;
        for (fs::directory_iterator task(entry->path() / "task", task_error), task_end;
             !task_error && task != task_end; task.increment(task_error)) {
            const auto tid = numeric_pid(task->path().filename().string());
            cpu_set_t current, target;
            if (tid <= 0 || sched_getaffinity(tid, sizeof(current), &current)) continue;
            CPU_AND(&target, &current, &preferred);
            // Preserve narrower application/cgroup restrictions and tolerate exits.
            if (CPU_COUNT(&target) && !CPU_EQUAL(&current, &target) &&
                sched_setaffinity(tid, sizeof(target), &target) && errno != ESRCH) {
                static bool warned = false;
                if (!warned) std::perror("Apply video GPU CPU preference");
                warned = true;
            }
        }
    }
    return found;
}
struct PowerHold {
    sd_bus *bus = nullptr;
    bool attempted = false;
    ~PowerHold() { sd_bus_flush_close_unref(bus); }
    void acquire() {
        if (attempted) return;
        attempted = true;
        int result = sd_bus_open_system(&bus);
        if (result < 0) { std::fprintf(stderr, "Video performance profile unavailable\n"); return; }
        sd_bus_set_method_call_timeout(bus, 3000000);
        sd_bus_set_allow_interactive_authorization(bus, 0);
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *reply = nullptr;
        result = sd_bus_call_method(bus, "org.freedesktop.UPower.PowerProfiles",
                                   "/org/freedesktop/UPower/PowerProfiles",
                                   "org.freedesktop.UPower.PowerProfiles", "HoldProfile",
                                   &error, &reply, "sss", "performance", "4K video playback",
                                   "org.uke.VideoSession");
        if (result < 0)
            std::fprintf(stderr, "Video performance profile unavailable: %s\n",
                         error.message ? error.message : "D-Bus request failed");
        else
            std::fprintf(stderr, "Video performance profile held for this Chrome session\n");
        sd_bus_error_free(&error);
        sd_bus_message_unref(reply);
    }
};
} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::puts("uke-video-session 1.0.0");
        return 0;
    }
    const std::string url = argc == 1 ? "https://www.youtube.com/" : argv[1];
    if (argc > 2 || !uke::youtube_url(url)) {
        std::fprintf(stderr, "Usage: uke-video-session [https://www.youtube.com/... | https://youtu.be/...]\n");
        return 2;
    }
    cpu_set_t allowed;
    if (sched_getaffinity(0, sizeof(allowed), &allowed)) {
        std::perror("Read allowed CPUs");
        return 1;
    }
    std::array<unsigned, CPU_SETSIZE> capacity{};
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        if (CPU_ISSET(cpu, &allowed))
            std::ifstream("/sys/devices/system/cpu/cpu" + std::to_string(cpu) +
                          "/cpu_capacity") >> capacity[cpu];
    const auto preferred = uke::preferred_cpus(allowed, capacity);
    struct sigaction action{};
    action.sa_handler = signal_received;
    sigemptyset(&action.sa_mask);
    for (const auto signal : {SIGINT, SIGTERM, SIGHUP}) sigaction(signal, &action, nullptr);
    const auto browser = fork();
    if (browser < 0) { std::perror("Start Chrome"); return 1; }
    if (!browser) {
        for (const auto signal : {SIGINT, SIGTERM, SIGHUP}) std::signal(signal, SIG_DFL);
        // Explicit caller overrides remain available for bounded comparisons.
        if (!std::getenv("IRIS_VAAPI_COPY") && setenv("IRIS_VAAPI_COPY", "cpu", 1)) _exit(126);
        const auto app = "--app=" + url;
        execl("/usr/bin/google-chrome-stable", "google-chrome-stable",
              "--profile-directory=Default", app.c_str(), static_cast<char *>(nullptr));
        std::perror("Start original Chrome");
        _exit(127);
    }
    PowerHold power;
    int status = 0;
    for (;;) {
        const auto waited = waitpid(browser, &status, WNOHANG);
        if (waited == browser) break;
        if (waited < 0 && errno != EINTR) { std::perror("Wait for Chrome"); return 1; }
        if (received_signal) {
            const int signal = received_signal;
            received_signal = 0;
            kill(browser, signal);
        }
        if (hint_gpu(browser, preferred)) power.acquire();
        // This connection's disappearance releases its own hold, including SIGKILL.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
