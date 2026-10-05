// SPDX-License-Identifier: MIT
// Read-only prerequisite inspection. This never installs or selects a boot entry.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
namespace {
std::string read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return {};
    std::string result;
    char byte{};
    while (result.size() < 65536 && stream.get(byte)) result += byte;
    if (stream.get(byte)) throw std::runtime_error("Inspection input exceeds its size limit");
    return result;
}
bool compatible(const std::string& value, const std::string& wanted) {
    std::size_t begin = 0;
    while (begin < value.size()) {
        const auto end = value.find('\0', begin);
        if (end == std::string::npos) return false;
        if (value.substr(begin, end - begin) == wanted) return true;
        begin = end + 1;
    }
    return false;
}
bool present(const fs::path& root, const char* path) {
    return fs::is_directory(root / path);
}
}
int main(int argc, char** argv) {
    try {
        fs::path root = "/";
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Usage: uke-boot-status [--root EXTRACTED_ROOT]\n"
                         "Read-only Uke EFI/kernel/root command-line inspection.\n"
                         "Exit 0: all prerequisites observed; 2: prerequisites missing; 1: input error.\n";
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--root") root = argv[2];
        else if (argc != 1) throw std::runtime_error("Invalid arguments; use --help");
        if (!fs::is_directory(root)) throw std::runtime_error("Inspection root is missing");
        const auto dt = read(root / "sys/firmware/devicetree/base/compatible");
        auto release = read(root / "proc/sys/kernel/osrelease");
        if (!release.empty() && release.back() == '\n') release.pop_back();
        const auto cmdline = read(root / "proc/cmdline");
        const bool efi = present(root, "sys/firmware/efi");
        const bool uke = compatible(dt, "xiaomi,uke") && compatible(dt, "qcom,sm7675");
        const bool kernel = release == "7.2.9-senemos-uke";
        bool root_selected = false;
        std::size_t begin = 0;
        while (begin < cmdline.size()) {
            const auto end = cmdline.find_first_of(" \t\r\n", begin);
            const auto argument = cmdline.substr(begin, end - begin);
            if (argument == "root=PARTLABEL=uke_linux") root_selected = true;
            else if (argument.rfind("root=", 0) == 0) root_selected = false;
            if (end == std::string::npos) break;
            begin = end + 1;
        }
        auto boolean = [](bool value) { return value ? "true" : "false"; };
        std::cout << "{\"schema_version\":1,\"scope\":\"read-only-current-environment\","
                  << "\"efi_present\":" << boolean(efi)
                  << ",\"uke_compatible\":" << boolean(uke)
                  << ",\"selected_kernel_release\":" << boolean(kernel)
                  << ",\"uke_root_command_line\":" << boolean(root_selected)
                  << ",\"storage_verified\":false,\"hardware_acceptance_granted\":false}\n";
        return efi && uke && kernel && root_selected ? 0 : 2;
    } catch (const std::exception&) {
        // Never print captured device strings, private host paths or identifiers.
        std::cerr << "Uke boot inspection failed; check the input root and --help\n";
        return 1;
    }
}
