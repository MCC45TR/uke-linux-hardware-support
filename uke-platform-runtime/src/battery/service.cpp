// SPDX-License-Identifier: MIT
#include "collector.hpp"
#include <systemd/sd-bus.h>
#include <chrono>
#include <csignal>
#include <future>
#include <iostream>
#include <stdexcept>

namespace uke::battery {
namespace {
volatile sig_atomic_t stopping = 0;
void stop(int) { stopping = 1; }
struct State { std::string snapshot = R"({"schema_version":1,"status":"collecting","fields":{}})"; };
int snapshot(sd_bus_message *message, void *userdata, sd_bus_error *) {
    return sd_bus_reply_method_return(message, "s", static_cast<State *>(userdata)->snapshot.c_str());
}
int caps(sd_bus_message *message, void *, sd_bus_error *) {
    return sd_bus_reply_method_return(message, "s", capabilities().c_str());
}
constexpr sd_bus_vtable methods[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("GetSnapshot", "", "s", snapshot, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("GetCapabilities", "", "s", caps, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};
}
int serve() {
    sd_bus *bus = nullptr; sd_bus_slot *slot = nullptr; State state;
    signal(SIGTERM, stop); signal(SIGINT, stop);
    try {
        int ret = sd_bus_open_system(&bus);
        if (ret < 0) throw std::runtime_error("cannot connect to system bus");
        ret = sd_bus_add_object_vtable(bus, &slot, "/org/senemos/Uke/Battery1", "org.senemos.Uke.Battery1", methods, &state);
        if (ret < 0 || sd_bus_request_name(bus, "org.senemos.Uke.Battery1", 0) < 0)
            throw std::runtime_error("cannot register read-only battery service");
        auto next = std::chrono::steady_clock::now();
        ModelSession models("/var/lib/uke-battery");
        std::future<std::string> pending;
        while (!stopping) {
            if (!pending.valid() && std::chrono::steady_clock::now() >= next)
                pending = std::async(std::launch::async, [&models] { return collect("/sys", models); });
            if (pending.valid() && pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                // A failed collector stops the service; no stuck worker is replaced.
                state.snapshot = pending.get(); next = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            }
            do { ret = sd_bus_process(bus, nullptr); } while (ret > 0);
            if (ret < 0 || sd_bus_wait(bus, 100000) < 0) throw std::runtime_error("system bus disconnected");
        }
        if (pending.valid()) state.snapshot = pending.get();
        models.flush();
        sd_bus_slot_unref(slot); sd_bus_flush_close_unref(bus); return 0;
    } catch (const std::exception &e) {
        sd_bus_slot_unref(slot); sd_bus_flush_close_unref(bus);
        std::cerr << "uke-battery: " << e.what() << '\n'; return 1;
    }
}
}
