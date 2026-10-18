// SPDX-License-Identifier: MIT
// Standard class observations only; no module load, daemon activation or control writes.
#include "usb-port.hpp"
#include "usb-drm.hpp"
#include <systemd/sd-bus.h>
#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;
namespace {
std::string quoted(const std::string& value) {
    std::string out = "\"";
    static const char hex[] = "0123456789abcdef";
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 32 || c >= 127) {
            out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15];
        } else out += char(c);
    }
    return out + '"';
}
std::optional<std::string> read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::string result;
    char c;
    while (result.size() < 4096 && in.get(c)) result += c;
    if (in.get(c)) throw std::runtime_error("Class attribute exceeds 4096 bytes");
    while (!result.empty() && (result.back() == '\n' || result.back() == '\0')) result.pop_back();
    return result;
}
std::vector<fs::path> entries(const fs::path& path) {
    std::error_code error;
    std::vector<fs::path> result;
    for (fs::directory_iterator i(path, error), end; !error && i != end; i.increment(error)) {
        if (result.size() == 128) throw std::runtime_error("Class inventory exceeds 128 objects");
        result.push_back(i->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}
std::string attributes(const fs::path& object, const std::vector<std::string>& fields) {
    std::string result = "{";
    for (const auto& field : fields) {
        if (result.size() > 1) result += ',';
        const auto value = read(object / field);
        result += quoted(field) + ":" + (value ? quoted(*value) : "null");
    }
    return result + '}';
}
bool token(const std::string& value, const std::string& wanted) {
    for (std::size_t pos = 0; pos < value.size();) {
        const auto end = value.find('\0', pos);
        if (value.substr(pos, end == std::string::npos ? end : end - pos) == wanted) return true;
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return false;
}
std::string inventory(const fs::path& root) {
    timespec start{}, finish{};
    if (clock_gettime(CLOCK_BOOTTIME, &start)) throw std::runtime_error("Cannot read boottime");
    const auto compatible = read(root / "sys/firmware/devicetree/base/compatible");
    const bool uke = compatible && token(*compatible, "xiaomi,uke") && token(*compatible, "qcom,sm7675");
    const auto release = read(root / "proc/sys/kernel/osrelease");
    std::string result = "{\"schema_id\":\"org.uke.usb.observation.v1\",\"record_kind\":\"class_snapshot\",";
    result += "\"target_identity_matches\":" + std::string(uke ? "true" : "false");
    result += ",\"kernel_release\":" + (release ? quoted(*release) : "null");
    result += ",\"coherence\":\"unproven_sequential_reads\",\"control_authorized\":false,";
    const auto auto_reason = read(root / "run/senemos-uke/usb-auto-state");
    result += "\"gadget_auto_reason\":" + (auto_reason ? quoted(*auto_reason) : "null") + ',';
    result += "\"is_live_measurement\":" + std::string(root == fs::path("/") ? "true," : "false,");
    result += "\"power_supply_units\":{\"voltage_now\":\"uV\",\"current_now\":\"uA\",\"temp\":\"0.1C\"},";
    result += "\"firmware_epoch\":null,\"attach_generation\":null,\"contract_generation\":null,";
    result += "\"requested\":null,\"applied\":null,\"verified\":null,\"classes\":{";
    const std::vector<std::pair<std::string, std::vector<std::string>>> groups = {
        {"typec", {"data_role", "power_role", "vconn_source", "orientation", "power_operation_mode", "accessory_mode"}},
        {"usb_role", {"role"}}, {"udc", {"state", "current_speed", "maximum_speed", "function"}},
        {"drm", {"status", "enabled", "modes"}}
    };
    bool first_group = true;
    for (const auto& group : groups) {
        if (!first_group) result += ',';
        first_group = false;
        result += quoted(group.first) + ":[";
        bool first = true;
        for (const auto& object : entries(root / "sys/class" / group.first)) {
            const auto name = object.filename().string();
            if (group.first == "drm" && name.find("-DP-") == std::string::npos) continue;
            if (!first) result += ',';
            first = false;
            result += "{\"class_name\":" + quoted(name) + ",\"provider\":";
            std::error_code error;
            const auto driver = fs::read_symlink(object / "device/driver", error);
            result += error ? "null" : quoted(driver.filename().string());
            result += ",\"attributes\":" + attributes(object, group.second) + '}';
        }
        result += ']';
    }
    result += ",\"power_supply\":[";
    bool first = true;
    for (const auto& object : entries(root / "sys/class/power_supply")) {
        const auto name = object.filename().string();
        const bool ucsi = name.rfind("ucsi-source-psy-", 0) == 0;
        if (!ucsi && name != "qcom-battmgr-bat" && name != "qcom-battmgr-usb") continue;
        if (!first) result += ',';
        first = false;
        result += "{\"class_name\":" + quoted(name) + ",\"origin\":";
        result += quoted(ucsi ? "ucsi_contract_derived_not_adc" : "battery_driver_reported_not_calibrated_here");
        result += ",\"attributes\":" + attributes(object,
            {"type", "present", "online", "status", "usb_type", "voltage_now", "current_now", "input_current_limit", "capacity", "temp"}) + '}';
    }
    std::string wcd = "[";
    for (const auto& object : entries(root / "sys/bus/i2c/devices")) {
        std::error_code error;
        const auto driver = fs::read_symlink(object / "driver", error);
        if (error || driver.filename() != "wcd939x-usb-uke") continue;
        if (wcd.size() > 1) wcd += ',';
        wcd += "{\"driver\":\"wcd939x-usb-uke\",\"cached_attributes\":" +
            attributes(object, {"silicon_revision"}) + '}';
    }
    const auto kms = root == fs::path("/") && uke ? uke::usb::kms_observation() : "null";
    if (clock_gettime(CLOCK_BOOTTIME, &finish)) throw std::runtime_error("Cannot read boottime");
    const auto ms = [](const timespec& time) { return std::int64_t(time.tv_sec) * 1000 + time.tv_nsec / 1000000; };
    result += "]},\"wcd\":" + wcd + "],\"kms\":" + kms;
    result += ",\"boottime_start_ms\":" + std::to_string(ms(start));
    result += ",\"boottime_end_ms\":" + std::to_string(ms(finish));
    result += ",\"dp_link_training\":null,\"allocated_dp_lanes\":null,\"trained_dp_lanes\":null,";
    result += "\"source_profile\":null,\"physical_acceptance\":\"NOT_RUN\"}";
    if (result.size() > 262144) throw std::runtime_error("Inventory exceeds 256 KiB");
    return result;
}
int get_state(sd_bus_message* message, void*, sd_bus_error* error) {
    try { return sd_bus_reply_method_return(message, "s", inventory("/").c_str()); }
    catch (const std::exception& e) { return sd_bus_error_set(error, "org.senemos.Uke.USB1.InventoryFailed", e.what()); }
}
void serve() {
    static const sd_bus_vtable table[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("GetState", "", "s", get_state, SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_VTABLE_END
    };
    sd_bus* bus = nullptr;
    int rc = sd_bus_open_system(&bus);
    if (rc < 0) throw std::runtime_error("Cannot open system bus");
    try {
        rc = sd_bus_add_object_vtable(bus, nullptr, "/org/senemos/Uke/USB1", "org.senemos.Uke.USB1", table, nullptr);
        if (rc < 0 || sd_bus_request_name(bus, "org.senemos.Uke.USB1", 0) < 0)
            throw std::runtime_error("Cannot acquire USB observer interface");
        for (;;) {
            rc = sd_bus_process(bus, nullptr);
            if (rc < 0) throw std::runtime_error("System bus disconnected");
            if (rc == 0 && sd_bus_wait(bus, UINT64_MAX) < 0) throw std::runtime_error("System bus wait failed");
        }
    } catch (...) { sd_bus_unref(bus); throw; }
}
}
int main(int argc, char** argv) {
    try {
        fs::path root = "/";
        if (argc == 2 && std::string(argv[1]) == "--serve") { serve(); return 0; }
        if (argc == 3 && std::string(argv[1]) == "--root") root = argv[2];
        else if (argc != 1) throw std::runtime_error("Usage: uke-usb-port [--root EXTRACTED_ROOT | --serve]");
        if (!fs::is_directory(root)) throw std::runtime_error("Inventory root is missing");
        std::cout << inventory(root) << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << "uke-usb-port: " << e.what() << '\n'; return 1; }
}
