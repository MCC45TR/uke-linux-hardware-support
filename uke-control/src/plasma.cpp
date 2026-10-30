// SPDX-License-Identifier: MIT
#include "plasma.hpp"
#include <cstdlib>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <systemd/sd-json.h>

namespace uke::desktop {
namespace {
struct State { std::string rotation = "null", brightness = "null", output,policy="null"; };
State inspect_text() {
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
            if (value == "always") state.rotation = "true";
            else if (value == "never" || value == "inTabletMode") state.rotation = "false";
            if(value=="always"||value=="never"||value=="inTabletMode")state.policy='"'+value+'"';
        }
        if (line.starts_with("Automatic brightness: ")) {
            ++brightnesses;
            const auto value = line.substr(22);
            if (value == "supported, enabled") state.brightness = "true";
            else if (value == "supported, disabled") state.brightness = "false";
        }
    }
    if (panels != 1) return {};
    if (rotations != 1) state.rotation = "null";
    if (brightnesses != 1) state.brightness = "null";
    return state;
}
State inspect() {
    const auto response=execute({"/usr/bin/kscreen-doctor","--json"},5,65536);
    // Older doctor versions may reject --json. A successful malformed document
    // is never reinterpreted as a different valid capability contract.
    if(response.status)return inspect_text();
    sd_json_variant* raw=nullptr;
    if(sd_json_parse(response.output.c_str(),static_cast<sd_json_parse_flags_t>(0),&raw,nullptr,nullptr)<0)return {};
    std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)> document(raw,sd_json_variant_unref);
    auto* outputs=sd_json_variant_by_key(raw,"outputs");if(!sd_json_variant_is_array(outputs)||sd_json_variant_elements(outputs)>64)return {};
    State state;unsigned panels=0;bool panel_ready=false;
    for(size_t i=0;i<sd_json_variant_elements(outputs);++i) {
        auto* output=sd_json_variant_by_index(outputs,i);auto* name=sd_json_variant_by_key(output,"name");
        if(!sd_json_variant_is_string(name)||!std::regex_match(sd_json_variant_string(name),std::regex("DSI-[0-9]{1,3}")))continue;
        if(++panels!=1)return {};
        state.output=sd_json_variant_string(name);
        auto* connected=sd_json_variant_by_key(output,"connected");auto* enabled=sd_json_variant_by_key(output,"enabled");
        if(!sd_json_variant_is_boolean(connected)||!sd_json_variant_boolean(connected)||(sd_json_variant_is_boolean(enabled)&&!sd_json_variant_boolean(enabled)))continue;
        panel_ready=true;
        auto* policy=sd_json_variant_by_key(output,"autoRotatePolicy");
        if(sd_json_variant_is_unsigned(policy)||sd_json_variant_is_integer(policy)) {
            const auto value=sd_json_variant_integer(policy);
            if(value>=0&&value<=2){state.rotation=value==2?"true":"false";state.policy=value==0?"\"never\"":value==1?"\"inTabletMode\"":"\"always\"";}
        }
    }
    if(panels!=1)return {};
    // Current upstream serializes autoRotatePolicy but omits automaticBrightness.
    // Keep the existing bounded brightness adapter only for the same connector.
    if(panel_ready){const auto legacy=inspect_text();if(legacy.output==state.output)state.brightness=legacy.brightness;}
    return state;
}
}
std::string plasma_status() {
    const auto state = inspect();
    return "{\"schema\":\"org.uke.control.plasma.v1\",\"output\":" + (state.output.empty() ? "null" : '"' + state.output + '"') + ",\"rotation\":" + state.rotation +
        ",\"automatic_brightness\":" + state.brightness + ",\"rotation_policy\":"+state.policy+"}\n";
}
Result plasma_set(const std::string& feature, const std::string& value) {
    if ((feature != "rotation" && feature != "brightness") || (value != "on" && value != "off"))
        throw std::runtime_error("Usage: uke-control desktop rotation|brightness on|off");
    const auto state = inspect();
    if ((feature == "rotation" ? state.rotation : state.brightness) == "null")
        throw std::runtime_error("The current KDE session does not expose this panel control");
    const bool on = value == "on";
    const std::string setting = feature == "rotation" ?
        std::string("autoRotatePolicy.") + (on ? "always" : "never") :
        std::string("autoBrightness.") + (on ? "enable" : "disable");
    return execute({"/usr/bin/kscreen-doctor", "output." + state.output + '.' + setting}, 5, 65536);
}
}
