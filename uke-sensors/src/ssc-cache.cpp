// SPDX-License-Identifier: GPL-3.0-or-later
// Publish a complete native-label RAM copy; never modify factory persist.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unistd.h>
namespace fs = std::filesystem;
static void copy_input(const fs::path &source, const fs::path &destination) {
    auto type = fs::symlink_status(source);
    if (fs::is_directory(type)) {
        fs::create_directory(destination);
        fs::permissions(destination, fs::perms::owner_all);
        for (auto &entry : fs::directory_iterator(source)) copy_input(entry.path(), destination / entry.path().filename());
        return;
    }
    if (!fs::is_regular_file(type)) throw std::runtime_error("Unsupported SSC input file type");
    std::ifstream input(source, std::ios::binary);
    std::ofstream output(destination, std::ios::binary);
    if (!input || !output) throw std::runtime_error("Cannot open SSC cache input/output");
    char buffer[8192];
    while (input.read(buffer, sizeof(buffer)) || input.gcount()) output.write(buffer, input.gcount());
    output.close();
    if (input.bad() || !output) throw std::runtime_error("Cannot copy complete SSC input");
    fs::permissions(destination, fs::perms::owner_read | fs::perms::owner_write);
    std::ifstream original(source, std::ios::binary), copied(destination, std::ios::binary);
    if (!original || !copied || !std::equal(std::istreambuf_iterator<char>(original), {},
                                          std::istreambuf_iterator<char>(copied), {}))
        throw std::runtime_error("SSC cache input comparison failed");
}
static void seed(const fs::path &root, const fs::path &base) {
    const auto state = base / "state";
    if (fs::exists(state)) {
        if (!fs::is_directory(fs::symlink_status(state)) || !fs::is_regular_file(state / "sensors/persist/sns_reg_version") ||
            !fs::is_regular_file(state / "sensors/config/json.lst") || !fs::is_regular_file(state / "dsp/adsp/sns_tppe.so"))
            throw std::runtime_error("Incomplete existing SSC state");
        return;
    }
    auto pattern = (base / ".seed.XXXXXX").string();
    if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create private SSC cache staging directory");
    const fs::path staging = pattern;
    try {
        fs::create_directories(staging / "sensors");
        fs::create_directories(staging / "dsp/adsp");
        fs::create_directory(staging / "socinfo");
        copy_input(root / "mnt/vendor/persist/sensors/registry", staging / "sensors/persist");
        copy_input(root / "usr/share/uke-sensors/odm", staging / "sensors/config");
        copy_input(root / "usr/share/uke-sensors/sns_reg.conf", staging / "sensors/sns_reg.conf");
        copy_input(root / "usr/lib/firmware/qcom/uke/sensors/sns_tppe.so", staging / "dsp/adsp/sns_tppe.so");
        for (const char *name : {"hw_platform", "platform_subtype", "platform_subtype_id", "platform_version", "soc_id", "revision"})
            copy_input(root / "sys/devices/soc0" / name, staging / "socinfo" / name);
        if (!fs::is_regular_file(staging / "sensors/config/json.lst") ||
            !fs::is_regular_file(staging / "sensors/persist/sns_reg_version"))
            throw std::runtime_error("Required SSC registry/configuration is absent");
        fs::rename(staging, state);
    } catch (...) {
        fs::remove_all(staging);
        throw;
    }
}
#ifndef UKE_SSC_CACHE_TEST
int main() {
    try { seed("/", "/run/uke-ssc"); return 0; }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif
