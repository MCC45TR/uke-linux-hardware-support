// SPDX-License-Identifier: MIT
#include "plasma.hpp"
#include <cstdlib>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace uke::desktop {
namespace {
struct State { std::string rotation = "null", brightness = "null", output, rotation_policy; };
State inspect() {
    // kscreen-doctor's human-readable capability names are English.
    setenv("LC_ALL", "C", 1);
    const auto result = execute({"/usr/bin/kscreen-doctor", "-o"}, 5, 65536);
    if (result.status) return {};
    const auto plain = std::regex_replace(result.output, std::regex("\x1b\\[[0-9;]*m"), "");
    std::istringstream lines(plain);
    State state;
    bool panel = false;
    unsigned panels = 0, rotations = 0, brightnesses = 0;
    for (std::string line; std::getline(lines, line);) {
        if (line.starts_with("Output:")) {
            std::smatch match;
            panel = std::regex_search(line, match, std::regex("^Output: +[0-9]+ +(DSI-[0-9]{1,3})( |$)"));
            if (panel) { ++panels; state.output = match[1]; }
        }
        if (!panel) continue;
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        line = line.substr(first);
        line.erase(line.find_last_not_of(" \t\r") + 1);
        if (line.starts_with("Auto Rotate Policy: ")) {
            ++rotations;
            const auto value = line.substr(20);
            if (value == "always" || value == "never" || value == "inTabletMode") {
                state.rotation_policy = value;
                state.rotation = value == "always" ? "true" : "false";
            }
        }
        if (line.starts_with("Automatic brightness: ")) {
            ++brightnesses;
            const auto value = line.substr(22);
            if (value == "supported, enabled") state.brightness = "true";
            else if (value == "supported, disabled") state.brightness = "false";
        }
    }
    if (panels != 1) return {};
    if (rotations != 1) { state.rotation = "null"; state.rotation_policy.clear(); }
    if (brightnesses != 1) state.brightness = "null";
    return state;
}
}
std::string plasma_status() {
    const auto state = inspect();
    return "{\"schema\":\"org.uke.control.plasma.v1\",\"output\":" + (state.output.empty() ? "null" : '"' + state.output + '"') + ",\"rotation\":" + state.rotation +
        ",\"automatic_brightness\":" + state.brightness + ",\"rotation_policy\":" +
        (state.rotation_policy.empty() ? "null" : '"' + state.rotation_policy + '"') + "}\n";
}
Result plasma_set(const std::string& feature, const std::string& value) {
    if ((feature != "rotation" && feature != "brightness") || (value != "on" && value != "off"))
        throw std::runtime_error("Usage: uke-control desktop rotation|brightness on|off");
    const auto state = inspect();
    if ((feature == "rotation" ? state.rotation : state.brightness) == "null")
        throw std::runtime_error("The current KDE session does not expose this panel control");
    const bool on = value == "on";
    const std::string wanted = feature == "rotation" ? (on ? "always" : "never") : (on ? "true" : "false");
    const auto matches = [&](const State& current) {
        return current.output == state.output &&
            (feature == "rotation" ? current.rotation_policy : current.brightness) == wanted;
    };
    const bool noop = matches(state);
    if (!noop) {
        const std::string setting = feature == "rotation" ? "autoRotatePolicy." + wanted :
            std::string("autoBrightness.") + (on ? "enable" : "disable");
        const auto result = execute({"/usr/bin/kscreen-doctor", "output." + state.output + '.' + setting}, 5, 65536);
        if (result.status) return result;
        const auto after = inspect();
        if (!matches(after)) throw std::runtime_error("KDE panel policy readback did not match; inspect the current desktop state");
    }
    return {0, "{\"schema\":\"org.uke.control.plasma.request.v1\",\"output\":\"" + state.output +
        "\",\"feature\":\"" + feature + "\",\"requested\":\"" + value +
        "\",\"already_matching\":" + (noop ? "true" : "false") +
        ",\"policy_readback_matched\":true,\"physical_validation\":\"NOT_ESTABLISHED\"}\n"};
}
}
