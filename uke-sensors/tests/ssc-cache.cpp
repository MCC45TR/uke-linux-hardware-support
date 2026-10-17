// SPDX-License-Identifier: GPL-3.0-or-later
#define UKE_SSC_CACHE_TEST
#include "../src/ssc-cache.cpp"
#include <cassert>
static void write(const fs::path &path, const char *value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path) << value;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    fs::path root = argv[1], base = root / "run/uke-ssc";
    fs::create_directories(base);
    write(root / "mnt/vendor/persist/sensors/registry/registry/calibration", "factory calibration");
    write(root / "mnt/vendor/persist/sensors/registry/sns_reg_version", "1");
    write(root / "usr/share/uke-sensors/odm/json.lst", "config");
    write(root / "usr/share/uke-sensors/sns_reg.conf", "profile");
    write(root / "usr/lib/firmware/qcom/uke/sensors/sns_tppe.so", "DSP fixture");
    bool rejected = false;
    try { seed(root, base); } catch (...) { rejected = true; }
    assert(rejected && !fs::exists(base / "state") && fs::is_empty(base));
    for (const char *name : {"hw_platform", "platform_subtype", "platform_subtype_id", "platform_version", "soc_id", "revision"})
        write(root / "sys/devices/soc0" / name, "field\n");
    fs::create_symlink("/etc/passwd", root / "usr/share/uke-sensors/odm/foreign");
    rejected = false;
    try { seed(root, base); } catch (...) { rejected = true; }
    assert(rejected && !fs::exists(base / "state") && fs::is_empty(base));
    fs::remove(root / "usr/share/uke-sensors/odm/foreign");
    seed(root, base);
    auto calibration = base / "state/sensors/persist/registry/calibration";
    assert(fs::status(calibration).permissions() == (fs::perms::owner_read | fs::perms::owner_write));
    write(calibration, "RAM-only change");
    seed(root, base);
    std::ifstream cached(calibration), original(root / "mnt/vendor/persist/sensors/registry/registry/calibration");
    std::string text;
    std::getline(cached, text); assert(text == "RAM-only change");
    std::getline(original, text); assert(text == "factory calibration");
    std::cout << "SSC cache: incomplete copy and symlinks rejected, atomic publication, private modes and RAM-only restart state passed\n";
}
