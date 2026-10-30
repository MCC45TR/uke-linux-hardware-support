// SPDX-License-Identifier: MIT
#include "uke-video-activity.hpp"
#include "uke-video-protocol.hpp"
#include <systemd/sd-bus.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace irisva {
namespace {
std::int64_t now_usec() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
std::atomic<std::uint64_t> next_stream{1};
}
struct VideoActivity::Impl {
    const unsigned width, height;
    const std::uint64_t stream = next_stream.fetch_add(1);
    std::atomic<std::int64_t> touched{0};
    std::atomic<bool> stop{false};
    std::thread worker;
    Impl(unsigned w, unsigned h) : width(w), height(h), worker([this] { run(); }) {}
    ~Impl() { stop.store(true); worker.join(); }
    void run() noexcept {
        sd_bus *bus = nullptr;
        bool active = false, warned = false;
        std::int64_t last_report = 0, retry = 0;
        while (!stop.load()) {
            const auto now = now_usec(), last = touched.load(std::memory_order_relaxed);
            const bool recent = last && now - last < 800000;
            if (!bus && recent && now >= retry) {
                if (sd_bus_open_user(&bus) < 0) {
                    bus = nullptr;
                    retry = now + 5000000;
                    if (!warned && std::getenv("IRIS_VAAPI_DEBUG"))
                        std::fprintf(stderr, "iris-vaapi: user video policy bus unavailable\n");
                    warned = true;
                }
            }
            if (bus) {
                int result = 0;
                for (int count = 0; count < 8; ++count) {
                    result = sd_bus_process(bus, nullptr);
                    if (result <= 0) break;
                }
                if (result < 0) {
                    sd_bus_close_unref(bus); bus = nullptr; active = false; retry = now + 1000000;
                } else if ((recent && now - last_report >= 500000) || (!recent && active)) {
                    sd_bus_message *message = nullptr;
                    result = sd_bus_message_new_method_call(bus, &message, uke::video::name,
                        uke::video::path, uke::video::name, "Report");
                    if (result >= 0) result = sd_bus_message_append(message, "tuub", stream, width, height, int(recent));
                    if (result >= 0) result = sd_bus_message_set_expect_reply(message, 0);
                    if (result >= 0) result = sd_bus_send(bus, message, nullptr);
                    sd_bus_message_unref(message);
                    if (result >= 0) { active = recent; last_report = now; }
                }
            }
            // No D-Bus call or wait runs on the decoder's submission/completion path.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // Connection loss immediately retires this context's server-side activity.
        sd_bus_close_unref(bus);
    }
};
VideoActivity::VideoActivity(unsigned width, unsigned height) : impl_(new Impl(width, height)) {}
VideoActivity::~VideoActivity() = default;
void VideoActivity::touch() noexcept { impl_->touched.store(now_usec(), std::memory_order_relaxed); }
}
