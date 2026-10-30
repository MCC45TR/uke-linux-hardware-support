// SPDX-License-Identifier: MIT
#include "wake.hpp"
#include "sysfs.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>
namespace fs = std::filesystem;
namespace uke::wake {
bool applicable(const fs::path& sys) {
    const auto file = sysfs::resolved(sys, sys/"firmware/devicetree/base/compatible");
    if (!file || !fs::is_regular_file(*file)) return false;
    std::array<char, 4097> buffer{};
    std::ifstream input(*file, std::ios::binary); input.read(buffer.data(), buffer.size());
    const auto size = input.gcount();
    if (size < 1 || size > 4096 || buffer[size - 1]) return false;
    const std::string_view value(buffer.data(), size);
    bool board = false, soc = false;
    for (size_t start = 0; start < value.size();) {
        const auto end = value.find('\0', start);
        if (end == value.npos) return false;
        const auto token = value.substr(start, end - start);
        board |= token == "xiaomi,uke"; soc |= token == "qcom,sm7675"; start = end + 1;
    }
    return board && soc;
}
std::vector<Source> sources(const fs::path& sys) {
    std::vector<Source> result{
        {"power-key", "wake_permission_unavailable", {}},
        {"volume-up", "wake_permission_unavailable", {}},
        {"volume-down", "wake_permission_unavailable", {}},
        {"usb", "shared_attach_remove_permission_unavailable", {}},
        {"bluetooth-uart", "wake_permission_unavailable", {}},
        {"charge-connect", "independent_firmware_wake_filter_unqualified", {}},
        {"charge-disconnect", "independent_firmware_wake_filter_unqualified", {}},
        {"double-tap", "touch_gesture_wake_not_implemented", {}},
        {"lid-open", "selective_lid_wake_not_implemented", {}},
        {"usb-host", "wake_permission_unavailable", {}}
    };
    const auto devices = sysfs::resolved(sys, sys/"devices");
    if (!devices) return result;
    auto admit = [&](Source& source, const fs::path& path) {
        const auto target = sysfs::resolved(*devices, path);
        if (!target) return;
        const auto value = sysfs::attribute(sys, *target);
        if (value != "\"enabled\"" && value != "\"disabled\"") return;
        source.path = *target; source.enabled = value == "\"enabled\""; source.reason.clear();
    };
    const std::array names{"pmic_pwrkey", "gpio-keys", "pmic_resin"};
    const std::array keys{"10000000000000 0", "8000000000000 0", "4000000000000 0"};
    std::array<unsigned, 3> matches{};
    for (const auto& input : sysfs::entries(sys, sys/"class/input")) {
        const auto leaf = input.filename().string();
        if (!leaf.starts_with("input") || leaf.size() == 5 || leaf.find_first_not_of("0123456789", 5) != std::string::npos) continue;
        const auto name = sysfs::attribute(sys, input/"name");
        for (unsigned i = 0; i < names.size(); ++i) if (name == sysfs::quote(names[i])) {
            ++matches[i];
            if (sysfs::attribute(sys, input/"capabilities/key") == sysfs::quote(keys[i]))
                admit(result[i], input/"device/power/wakeup");
        }
    }
    for (unsigned i = 0; i < names.size(); ++i) if (matches[i] > 1) {
        result[i].path.clear(); result[i].enabled = false; result[i].reason = "ambiguous_input_provider";
    }
    admit(result[3], sys/"bus/platform/devices/a600000.ssusb/power/wakeup");
    admit(result[4], sys/"bus/platform/devices/888000.qcom,qup_uart/power/wakeup");
    unsigned touch_matches = 0;
    for (const auto& touch : sysfs::entries(sys, sys/"bus/spi/devices")) {
        const auto driver = sysfs::driver(sys,touch);
        const bool current = driver == "\"nt36532-uke-touch\"" &&
            sysfs::attribute(sys,touch/"of_node/compatible") == "\"novatek,nt36532-uke-touch\"";
        if (!current && driver != "\"nt36532-uke-thp\"") continue;
        ++touch_matches;
        const auto armed = sysfs::attribute(sys,touch/"gesture_wake_armed");
        if (armed == "\"0\"" || armed == "\"1\"") admit(result[7],touch/"power/wakeup");
    }
    if (touch_matches > 1) {
        result[7].path.clear(); result[7].enabled = false; result[7].reason = "ambiguous_touch_provider";
    }
    admit(result[8], sys/"bus/platform/devices/hall-lid/power/wakeup");
    const auto controller = sysfs::resolved(*devices,
        sys/"bus/platform/devices/a600000.ssusb");
    unsigned hosts = 0;
    const auto host_driver = sysfs::resolved(sys, sys/"bus/platform/drivers/xhci-hcd");
    if (controller && host_driver) for (const auto& entry : sysfs::entries(sys, *controller)) {
        const auto leaf = entry.filename().string();
        if (!leaf.starts_with("xhci-hcd.") || !leaf.ends_with(".auto") || leaf.size() <= 14 ||
            leaf.substr(9, leaf.size()-14).find_first_not_of("0123456789") != leaf.npos)
            continue;
        const auto target = sysfs::resolved(*controller, entry);
        if (!target || sysfs::resolved(sys, entry/"driver") != host_driver) continue;
        ++hosts;
        admit(result[9], *target/"power/wakeup");
    }
    if (hosts > 1) {
        result[9].path.clear(); result[9].enabled = false;
        result[9].reason = "ambiguous_usb_host_provider";
    }
    if (controller) for (const auto& entry : sysfs::entries(sys, sys/"bus/usb/devices")) {
        const auto leaf = entry.filename().string();
        if (leaf.empty() || leaf.find_first_not_of("0123456789.usb-") != leaf.npos)
            continue;
        const auto target = sysfs::resolved(*controller, entry);
        if (!target || !fs::is_regular_file(*target/"idVendor"))
            continue;
        Source source{"usb-device-"+leaf, "remote_wakeup_not_advertised", {}};
        admit(source, *target/"power/wakeup");
        result.push_back(std::move(source));
    }
    return result;
}
bool recovery_key(const std::vector<Source>& sources) {
    return std::any_of(sources.begin(), sources.end(), [](const Source& source) {
        return !source.path.empty() && source.enabled &&
            (source.name == "power-key" || source.name == "volume-up" || source.name == "volume-down");
    });
}
std::string snapshot(const fs::path& sys) {
    return snapshot(sys,{});
}
std::string snapshot(const fs::path& sys,const std::string& selected) {
    std::string result = std::string("{\"scope\":\"device_wake_permission\",\"board_compatible\":") +
        (applicable(sys) ? "true" : "false") + ",\"physical_acceptance\":null,\"sources\":[";
    bool comma = false,matched=selected.empty();
    for (const auto& source : sources(sys)) {
        if(!selected.empty()&&source.name!=selected)continue;
        matched=true;
        if (comma) result += ',';
        comma = true;
        result += "{\"name\":" + sysfs::quote(source.name) + ",\"control_available\":" + (source.path.empty() ? "false" : "true") +
            ",\"enabled\":" + (source.path.empty() ? "null" : source.enabled ? "true" : "false") +
            ",\"reason\":" + (source.reason.empty() ? "null" : sysfs::quote(source.reason));
        if(source.name=="double-tap") {
            const auto armed=source.path.empty()?"null":sysfs::attribute(sys,source.path.parent_path().parent_path()/"gesture_wake_armed");
            result+=",\"gesture_armed\":"+std::string(armed=="\"1\""?"true":armed=="\"0\""?"false":"null");
        }
        result+='}';
    }
    if(!matched)throw std::runtime_error("Unknown wake source");
    return result + "]}\n";
}
}
