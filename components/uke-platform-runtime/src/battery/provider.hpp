// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <system_error>

namespace uke::battery::provider {
// auxiliary_driver_register prefixes the driver's name with KBUILD_MODNAME.
inline constexpr char bound_driver_name[] = "qcom_battmgr.pmic_glink_power_supply";
inline std::filesystem::path find(const std::filesystem::path &supply,
                                  const std::filesystem::path &sysfs_root = "/sys") {
    namespace fs = std::filesystem;
    std::error_code error;
    const auto expected = fs::canonical(sysfs_root / "bus/auxiliary/drivers" / bound_driver_name, error);
    if (error) return {};
    auto node = fs::canonical(supply / "device", error);
    for (int i = 0; !error && i != 5 && node != node.root_path(); ++i) {
        const auto driver = fs::canonical(node / "driver", error);
        if (!error && driver == expected) return node;
        error.clear(); node = node.parent_path();
    }
    return {};
}
}
