// SPDX-License-Identifier: MIT
#include "affinity.hpp"
#include "policy.hpp"
#include <systemd/sd-bus.h>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>

namespace {
volatile sig_atomic_t stopping = 0;
void stop(int) { stopping = 1; }
std::uint64_t now_usec() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct PowerHold {
    sd_bus *bus = nullptr;
    bool held = false, attempted = false, suppressed = false, performance = false;
    std::uint32_t cookie = 0;
    ~PowerHold() { idle(); }
    static int released(sd_bus_message *message, void *data, sd_bus_error *) {
        auto &self = *static_cast<PowerHold *>(data);
        std::uint32_t cookie = 0;
        if (sd_bus_message_read(message, "u", &cookie) > 0 && self.held && cookie == self.cookie) {
            self.held = false;
            self.performance = false;
            self.suppressed = true;
            std::fprintf(stderr, "Video policy respects the manual power-profile change\n");
        }
        return 0;
    }
    static int properties(sd_bus_message *message, void *data, sd_bus_error *) {
        auto &self = *static_cast<PowerHold *>(data);
        const char *interface = nullptr;
        if (sd_bus_message_read(message, "s", &interface) < 0 ||
            std::strcmp(interface, "org.freedesktop.UPower.PowerProfiles")) return 0;
        if (sd_bus_message_enter_container(message, 'a', "{sv}") < 0) return 0;
        while (sd_bus_message_enter_container(message, 'e', "sv") > 0) {
            const char *key = nullptr;
            if (sd_bus_message_read(message, "s", &key) < 0) return 0;
            if (!std::strcmp(key, "ActiveProfile")) {
                const char *profile = nullptr;
                if (sd_bus_message_enter_container(message, 'v', "s") < 0 ||
                    sd_bus_message_read(message, "s", &profile) < 0) return 0;
                self.performance = !std::strcmp(profile, "performance");
                sd_bus_message_exit_container(message);
            } else sd_bus_message_skip(message, "v");
            sd_bus_message_exit_container(message);
        }
        return 0;
    }
    void acquire() {
        if (attempted || suppressed) return;
        attempted = true;
        if (sd_bus_open_system(&bus) < 0) return;
        sd_bus_set_method_call_timeout(bus, 500000);
        sd_bus_set_allow_interactive_authorization(bus, 0);
        sd_bus_match_signal(bus, nullptr, "org.freedesktop.UPower.PowerProfiles",
                            "/org/freedesktop/UPower/PowerProfiles",
                            "org.freedesktop.UPower.PowerProfiles", "ProfileReleased", released, this);
        sd_bus_match_signal(bus, nullptr, "org.freedesktop.UPower.PowerProfiles",
                            "/org/freedesktop/UPower/PowerProfiles",
                            "org.freedesktop.DBus.Properties", "PropertiesChanged", properties, this);
        char *selected = nullptr;
        if (sd_bus_get_property_string(bus, "org.freedesktop.UPower.PowerProfiles",
            "/org/freedesktop/UPower/PowerProfiles", "org.freedesktop.UPower.PowerProfiles",
            "ActiveProfile", nullptr, &selected) >= 0 && !std::strcmp(selected, "power-saver")) {
            std::free(selected);
            suppressed = true;
            return;
        }
        std::free(selected);
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *reply = nullptr;
        const int result = sd_bus_call_method(bus, "org.freedesktop.UPower.PowerProfiles",
            "/org/freedesktop/UPower/PowerProfiles", "org.freedesktop.UPower.PowerProfiles",
            "HoldProfile", &error, &reply, "sss", "performance", "Active UHD video decoding",
            uke::video::name);
        held = result >= 0 && sd_bus_message_read(reply, "u", &cookie) > 0;
        if (!held) std::fprintf(stderr, "Video performance hold unavailable: %s\n",
                               error.message ? error.message : "D-Bus request failed");
        sd_bus_error_free(&error);
        sd_bus_message_unref(reply);
        if (held) {
            char *profile = nullptr;
            if (sd_bus_get_property_string(bus, "org.freedesktop.UPower.PowerProfiles",
                "/org/freedesktop/UPower/PowerProfiles", "org.freedesktop.UPower.PowerProfiles",
                "ActiveProfile", nullptr, &profile) >= 0) performance = !std::strcmp(profile, "performance");
            std::free(profile);
        }
    }
    void process() {
        if (!bus) return;
        for (int count = 0; count < 32; ++count) {
            const int result = sd_bus_process(bus, nullptr);
            if (result < 0) {
                sd_bus_flush_close_unref(bus);
                bus = nullptr;
                held = performance = false;
                break;
            }
            if (!result) break;
        }
    }
    void idle() {
        if (bus && held) sd_bus_call_method(bus, "org.freedesktop.UPower.PowerProfiles",
            "/org/freedesktop/UPower/PowerProfiles", "org.freedesktop.UPower.PowerProfiles",
            "ReleaseProfile", nullptr, nullptr, "u", cookie);
        sd_bus_flush_close_unref(bus);
        bus = nullptr;
        held = attempted = suppressed = performance = false;
    }
};
struct App {
    uke::ActivitySet activity;
    PowerHold power;
};
int report(sd_bus_message *message, void *data, sd_bus_error *error) {
    auto &app = *static_cast<App *>(data);
    std::uint64_t stream = 0;
    std::uint32_t width = 0, height = 0;
    int active = 0;
    int result = sd_bus_message_read(message, "tuub", &stream, &width, &height, &active);
    if (result < 0) return result;
    sd_bus_creds *credentials = nullptr;
    pid_t pid = -1;
    uid_t uid = ~uid_t{};
    result = sd_bus_query_sender_creds(message, SD_BUS_CREDS_PID | SD_BUS_CREDS_EUID, &credentials);
    if (result >= 0) {
        sd_bus_creds_get_pid(credentials, &pid);
        sd_bus_creds_get_euid(credentials, &uid);
    }
    sd_bus_creds_unref(credentials);
    if (uid != geteuid() || !uke::chrome_gpu(pid))
        return sd_bus_error_set_const(error, SD_BUS_ERROR_ACCESS_DENIED, "Only the user's original Chrome GPU process can report video activity");
    const char *sender = sd_bus_message_get_sender(message);
    if (!sender || !app.activity.report(sender, stream, pid, width, height, active, now_usec()))
        return sd_bus_error_set_const(error, SD_BUS_ERROR_LIMITS_EXCEEDED, "Invalid or excessive video activity");
    return sd_bus_reply_method_return(message, "");
}
int owner_changed(sd_bus_message *message, void *data, sd_bus_error *) {
    const char *name = nullptr, *old = nullptr, *current = nullptr;
    if (sd_bus_message_read(message, "sss", &name, &old, &current) > 0 && !*current)
        static_cast<App *>(data)->activity.remove_sender(name);
    return 0;
}
int property(sd_bus *, const char *, const char *, const char *name,
             sd_bus_message *reply, void *data, sd_bus_error *) {
    const auto &app = *static_cast<App *>(data);
    if (!std::strcmp(name, "ActiveStreams")) return sd_bus_message_append(reply, "u", app.activity.size());
    const int value = !std::strcmp(name, "Held") ? app.power.held : app.power.suppressed;
    return sd_bus_message_append(reply, "b", value);
}
const sd_bus_vtable methods[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Report", "tuub", "", report, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_PROPERTY("ActiveStreams", "u", property, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Held", "b", property, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Suppressed", "b", property, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_VTABLE_END
};
}
int main(int argc, char **argv) {
    if (argc == 2 && !std::strcmp(argv[1], "--version")) {
        std::puts("uke-video-session 1.1.0");
        return 0;
    }
    const bool restore_only = argc == 2 && !std::strcmp(argv[1], "--restore");
    if (argc > 1 && !restore_only) return 2;
    const char *runtime = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime || *runtime != '/') { std::fprintf(stderr, "A user runtime directory is required\n"); return 1; }
    const char *managed = std::getenv("RUNTIME_DIRECTORY");
    const std::string directory = managed && *managed == '/' ? managed : std::string(runtime) + "/uke-video-session";
    cpu_set_t allowed;
    if (sched_getaffinity(0, sizeof(allowed), &allowed)) return 1;
    std::array<unsigned, CPU_SETSIZE> capacity{};
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        if (CPU_ISSET(cpu, &allowed))
            std::ifstream("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpu_capacity") >> capacity[cpu];
    uke::AffinityHints hints(directory, uke::preferred_cpus(allowed, capacity), uke::chrome_gpu);
    if (!hints.recover()) { std::fprintf(stderr, "Video affinity recovery record is invalid or unavailable\n"); return 1; }
    if (restore_only) return 0;
    for (const auto signal : {SIGTERM, SIGINT, SIGHUP}) std::signal(signal, stop);
    sd_bus *session = nullptr;
    if (sd_bus_open_user(&session) < 0) return 1;
    sd_bus_set_method_call_timeout(session, 500000);
    App app;
    if (sd_bus_add_object_vtable(session, nullptr, uke::video::path, uke::video::name, methods, &app) < 0 ||
        sd_bus_match_signal(session, nullptr, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                            "org.freedesktop.DBus", "NameOwnerChanged", owner_changed, &app) < 0 ||
        sd_bus_request_name(session, uke::video::name, 0) < 0) {
        sd_bus_flush_close_unref(session);
        return 1;
    }
    std::uint64_t last_work = now_usec();
    unsigned old_count = 0;
    bool old_held = false, old_suppressed = false;
    int status = 0;
    while (!stopping) {
        for (int count = 0; count < 32; ++count) {
            const auto result = sd_bus_process(session, nullptr);
            if (result < 0) { stopping = 1; status = 1; break; }
            if (!result) break;
        }
        const auto now = now_usec();
        app.activity.expire(now);
        app.power.process();
        const auto processes = app.activity.processes();
        if (processes.empty()) app.power.idle(); else { last_work = now; app.power.acquire(); }
        if (!hints.update(app.power.held && app.power.performance && !app.power.suppressed ? processes : std::set<pid_t>{})) {
            std::fprintf(stderr, "Video affinity update failed; retaining recovery records\n");
            status = 1;
            break;
        }
        if (old_count != app.activity.size() || old_held != app.power.held || old_suppressed != app.power.suppressed) {
            sd_bus_emit_properties_changed(session, uke::video::path, uke::video::name,
                                           "ActiveStreams", "Held", "Suppressed", nullptr);
            old_count = app.activity.size(); old_held = app.power.held; old_suppressed = app.power.suppressed;
        }
        if (now - last_work >= uke::video::idle_exit_usec) break;
        sd_bus_wait(session, 100000);
    }
    hints.update({});
    app.power.idle();
    sd_bus_flush_close_unref(session);
    return status;
}
