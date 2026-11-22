// SPDX-License-Identifier: MIT
// Host-only fake sysfs links, using the actual shared resolver in both clients.
#include "../src/battery/provider.hpp"
#include <cassert>
#include <filesystem>
#include <string>
namespace fs = std::filesystem;
namespace provider = uke::battery::provider;
int main(int argc, char **argv) {
    assert(argc == 2);
    const fs::path root = fs::absolute(argv[1]);
    const auto drivers = root / "bus/auxiliary/drivers";
    const auto expected = drivers / provider::bound_driver_name;
    const auto node = root / "devices/platform/pmic-glink/pmic_glink.power-supply.0";
    const auto bat = root / "class/power_supply/qcom-battmgr-bat";
    const auto usb = root / "class/power_supply/qcom-battmgr-usb";
    fs::create_directories(expected); fs::create_directories(node / "child");
    fs::create_directories(bat); fs::create_directories(usb);
    fs::create_directory_symlink(node / "child", bat / "device");
    fs::create_directory_symlink(node, usb / "device");
    const auto bind = [&](const fs::path &driver) {
        fs::remove(node / "driver"); fs::create_directory_symlink(driver, node / "driver");
    };
    bind(expected);
    assert(provider::find(bat, root) == node && provider::find(usb, root) == node);
    for (const char *name : {"pmic_glink_power_supply", "other.pmic_glink_power_supply",
             "qcom_battmgr.pmic_glink_power_supply_extra", "qcom_battmgr.pmic_glink_power_supply.0"}) {
        fs::create_directories(drivers / name); bind(drivers / name);
        assert(provider::find(bat, root).empty() && provider::find(usb, root).empty());
    }
    const auto wrong_bus = root / "bus/platform/drivers" / provider::bound_driver_name;
    fs::create_directories(wrong_bus); bind(wrong_bus);
    assert(provider::find(bat, root).empty());
    bind(drivers / "missing"); assert(provider::find(bat, root).empty());
    bind(expected);
    const auto other = root / "devices/platform/other-auxiliary";
    fs::create_directories(other); fs::create_directory_symlink(expected, other / "driver");
    fs::remove(usb / "device"); fs::create_directory_symlink(other, usb / "device");
    assert(provider::find(bat, root) != provider::find(usb, root)); // Caller must reject the mismatched pair.
    fs::remove(bat / "device"); assert(provider::find(bat, root).empty());
}
