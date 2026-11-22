// SPDX-License-Identifier: MIT
// Build-worker fixture; never installed in a tablet package.
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    std::vector<int> values{4, 1, 3, 2};
    std::ranges::sort(values);
    if (values != std::vector<int>{1, 2, 3, 4}) return 1;
    auto result = std::async(std::launch::async, [] { return std::string("native"); });
    if (result.get() != "native") return 2;
    if (std::filesystem::path("a/b/../c").lexically_normal() != "a/c") return 3;
    try {
        throw std::runtime_error("fixture");
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) != "fixture") return 4;
    }
    const auto day = std::chrono::year{2026}/10/5;
    if (!day.ok()) return 5;
    std::cout << "Native GNU C++ runtime smoke fixture passed\n";
    return 0;
}
