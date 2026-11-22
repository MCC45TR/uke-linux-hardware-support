// SPDX-License-Identifier: MIT
// Read Uke identity without treating product specifications as driver results.
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
namespace {
std::string read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        if (fs::exists(path)) throw std::runtime_error("Input unreadable");
        return {};
    }
    std::string value;
    char byte{};
    while (stream.get(byte)) {
        if (value.size() == 65536) throw std::runtime_error("Input too large");
        value += byte;
    }
    if (stream.bad()) throw std::runtime_error("Input read failed");
    return value;
}
std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
bool compatible(const std::string& values, const std::string& wanted) {
    if (values.empty() || values.back() != '\0') return false;
    std::istringstream stream(values);
    std::string value;
    while (std::getline(stream, value, '\0')) if (value == wanted) return true;
    return false;
}
void validate(const std::string& value) {
    if (value.empty() || value.size() > 128 ||
        value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") != std::string::npos)
        throw std::runtime_error("Invalid identity value");
}
void merge(std::string& result, const std::string& value) {
    validate(value);
    if (!result.empty() && result != value) throw std::runtime_error("Conflicting identity values");
    result = value;
}
std::string parameter(const std::string& bootconfig, const std::string& cmdline,
                      const std::string& bootargs, const std::string& key) {
    std::string result, line;
    std::istringstream config(bootconfig);
    while (std::getline(config, line)) {
        const auto equal = line.find('=');
        if (equal == std::string::npos || trim(line.substr(0, equal)) != key) continue;
        std::istringstream stream(trim(line.substr(equal + 1)));
        std::string value, trailing;
        if (!(stream >> std::quoted(value)) || (stream >> trailing)) throw std::runtime_error("Invalid bootconfig value");
        merge(result, value);
    }
    for (const auto& source : {cmdline, bootargs}) {
        std::istringstream stream(source);
        while (stream >> line) {
            if (line.compare(0, key.size() + 1, key + '=') != 0) continue;
            std::istringstream field(line.substr(key.size() + 1));
            std::string value, trailing;
            if (!(field >> std::quoted(value)) || (field >> trailing)) throw std::runtime_error("Invalid command line value");
            merge(result, value);
        }
    }
    return result;
}
std::string dtString(const fs::path& path) {
    auto value = read(path);
    if (value.empty()) return {};
    if (value.back() != '\0' || value.find('\0') != value.size() - 1)
        throw std::runtime_error("Invalid device tree string");
    value.pop_back();
    return value;
}
}
int main(int argc, char** argv) {
    try {
        fs::path root = "/";
        bool show_serial = false, machine_info = false;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--root" && i + 1 < argc) root = argv[++i];
            else if (option == "--show-serial") show_serial = true;
            else if (option == "--machine-info") machine_info = true;
            else if (option == "--help" && argc == 2) {
                std::cout << "Usage: uke-hardware-info [--root EXTRACTED_ROOT] [--machine-info | --show-serial]\n"
                             "Read-only Uke identity; serial numbers are omitted unless explicitly requested.\n"
                             "Exit 0: Uke identity; 2: foreign or missing device; 1: invalid input.\n";
                return 0;
            } else throw std::runtime_error("Invalid arguments");
        }
        if (machine_info && show_serial) throw std::runtime_error("Serial cannot enter machine-info");
        const auto dt = root / "sys/firmware/devicetree/base";
        const auto ids = read(dt / "compatible");
        if (!compatible(ids, "xiaomi,uke") || !compatible(ids, "qcom,sm7675")) return 2;
        const auto config = read(root / "proc/bootconfig");
        const auto cmdline = read(root / "proc/cmdline");
        const auto bootargs = dtString(dt / "chosen/bootargs");
        const auto sku = parameter(config, cmdline, bootargs, "androidboot.product.hardware.sku");
        const auto revision = parameter(config, cmdline, bootargs, "androidboot.hwversion");
        const auto model = dtString(dt / "model");
        const bool poco = sku == "ukepgl" || (sku.empty() && model == "POCO Pad X1");
        const std::string product = poco ? "POCO Pad X1" : "Xiaomi Pad 7";
        const char* detection = sku == "ukepgl" ? "bootloader-sku" :
                                poco ? "device-tree-model" : "board-family-default";
        if (machine_info) {
            std::cout << "HARDWARE_VENDOR=\"Xiaomi Communications Co., Ltd.\"\n"
                      << "HARDWARE_MODEL=\"" << product << "\"\n"
                      << "HARDWARE_VERSION=\"Rev:1.1\"\n";
            return 0;
        }
        // Read factory serials only on request; SoC IDs, MACs and storage IDs are unrelated.
        std::string serial, serial_source;
        if (show_serial) {
            serial = parameter(config, cmdline, bootargs, "androidboot.serialno");
            if (!serial.empty()) serial_source = "bootloader-serialno";
            const auto dt_serial = dtString(dt / "serial-number");
            if (!dt_serial.empty()) {
                merge(serial, dt_serial);
                if (serial_source.empty()) serial_source = "device-tree-serial-number";
            }
            if (!serial.empty() && (serial == "unknown" || serial == "none" ||
                serial == "default" || serial.find_first_not_of('0') == std::string::npos)) {
                serial.clear();
                serial_source.clear();
            }
        }
        std::cout << "{\"schema_version\":1,\"device\":\"uke\",\"soc\":\"SM7675\","
                     "\"processor\":\"Qualcomm Snapdragon 7+ Gen 3\","
                     "\"graphics_processor\":\"Qualcomm Adreno 732\","
                     "\"manufacturer\":\"Xiaomi Communications Co., Ltd.\","
                  << "\"product_name\":\"" << product << "\",\"product_detection\":\"" << detection << "\","
                     "\"system_version\":\"Rev:1.1\",\"system_version_source\":\"requested-presentation-label\","
                  << "\"oem_hwversion\":" << (revision.empty() ? "null" : '"' + revision + '"') << ','
                  << "\"serial_number\":" << (serial.empty() ? "null" : '"' + serial + '"') << ','
                  << "\"serial_source\":" << (serial_source.empty() ? "null" : '"' + serial_source + '"') << ','
                  << "\"graphics_acceleration_tested\":false,\"hardware_acceptance_granted\":false}\n";
        return 0;
    } catch (const std::exception&) {
        std::cerr << "Uke hardware identity unavailable: invalid, conflicting or unreadable input\n";
        return 1;
    }
}
