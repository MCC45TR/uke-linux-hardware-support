// SPDX-License-Identifier: MIT
#include "collector.hpp"
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    try {
        std::string root = "/sys"; unsigned samples = 1, interval = 30; bool inventory = false, daemon = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--sysfs-root" && i + 1 < argc) root = argv[++i];
            else if (arg == "--watch" && i + 1 < argc) {
                const std::string count = argv[++i];
                auto parsed = std::from_chars(count.data(), count.data() + count.size(), samples);
                if (parsed.ec != std::errc{} || parsed.ptr != count.data() + count.size() || samples < 1 || samples > 120)
                    throw std::runtime_error("watch count must be 1..120");
            } else if (arg == "--interval-seconds" && i + 1 < argc) {
                const std::string value = argv[++i];
                auto parsed = std::from_chars(value.data(), value.data() + value.size(), interval);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || interval < 5 || interval > 60)
                    throw std::runtime_error("interval must be 5..60 seconds");
            } else if (arg == "--inventory") inventory = true;
            else if (arg == "--daemon") daemon = true;
            else if (arg == "--capabilities") { std::cout << uke::battery::capabilities() << '\n'; return 0; }
            else if (arg == "--help") {
                std::cout << "uke-battery [--inventory] [--watch 1..120] [--interval-seconds 5..60] [--sysfs-root DIR]\n"
                          << "uke-battery --capabilities | --daemon\nRead-only; no charging control. Default interval: 30 seconds.\n";
                return 0;
            } else throw std::runtime_error("unknown or incomplete option");
        }
        if (daemon) {
            if (root != "/sys" || inventory || samples != 1 || interval != 30) throw std::runtime_error("daemon requires the real sysfs provider and fixed cadence");
            return uke::battery::serve();
        }
        size_t bytes = 0;
        uke::battery::ModelSession models;
        for (unsigned i = 0; i < samples; ++i) {
            auto record = uke::battery::collect(root, models, inventory);
            bytes += record.size() + 1;
            if (bytes > 4 * 1024 * 1024) throw std::runtime_error("session output limit reached");
            std::cout << record << std::endl;
            if (i + 1 < samples) std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "uke-battery: " << e.what() << '\n'; return 1;
    }
}
