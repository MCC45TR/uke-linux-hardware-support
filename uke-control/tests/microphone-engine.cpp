// SPDX-License-Identifier: MIT
#include "microphone-engine.hpp"
#include <iostream>
int main() {
    const auto result = uke::microphone::verify_engine();
    std::cout << result << '\n';
    return result["runnable"].asBool() && result["processed_frames"] == 48000 ? 0 : 1;
}
