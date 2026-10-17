// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include "../src/ppd-driver-uke-cpufreq.cpp"
static std::string root;
static unsigned fail_policy = 99;
static bool bad_readback;
extern "C" char *ppd_utils_get_sysfs_path(const char *path) { return g_strdup((root + path).c_str()); }
extern "C" gboolean ppd_utils_write(const char *path, const char *value, GError **error) {
    if (strstr(path, ("policy" + std::to_string(fail_policy) + "/").c_str())) {
        fail_policy = 99;
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Injected write failure");
        return FALSE;
    }
    std::ofstream file(path);
    file << (bad_readback ? "wrong" : value);
    bad_readback = false;
    return bool(file);
}
static void write(const std::string &path, const std::string &value) {
    auto full = std::filesystem::path(root + path);
    std::filesystem::create_directories(full.parent_path());
    std::ofstream(full) << value;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    root = argv[1];
    const std::string base = "/sys/devices/system/cpu/cpufreq/policy";
    write("/sys/firmware/devicetree/base/compatible", std::string("xiaomi,uke\0qcom,sm7675\0", 23));
    for (unsigned policy : policies) {
        write(base + std::to_string(policy) + "/scaling_driver", "qcom-cpufreq-hw\n");
        write(base + std::to_string(policy) + "/scaling_available_governors", "powersave schedutil performance\n");
        write(base + std::to_string(policy) + "/scaling_governor", "schedutil\n");
    }
    g_autoptr(PpdDriverUkeCpufreq) driver = PPD_DRIVER_UKE_CPUFREQ(g_object_new(PPD_TYPE_DRIVER_UKE_CPUFREQ, nullptr));
    assert(ppd_driver_get_profiles(PPD_DRIVER(driver)) == (PPD_PROFILE_POWER_SAVER | PPD_PROFILE_BALANCED | PPD_PROFILE_PERFORMANCE));
    assert(probe(nullptr) == PPD_PROBE_RESULT_SUCCESS);
    for (auto profile : {PPD_PROFILE_POWER_SAVER, PPD_PROFILE_PERFORMANCE, PPD_PROFILE_BALANCED}) {
        g_autoptr(GError) error = nullptr;
        assert(activate(nullptr, profile, PPD_PROFILE_ACTIVATION_REASON_USER, &error));
        const char *expected = profile == PPD_PROFILE_POWER_SAVER ? "powersave" : profile == PPD_PROFILE_PERFORMANCE ? "performance" : "schedutil";
        for (unsigned policy : policies) assert(read_is(policy, "scaling_governor", expected));
    }
    for (bool readback : {false, true}) {
        g_autoptr(GError) error = nullptr;
        fail_policy = readback ? 99 : 3; bad_readback = readback;
        assert(!activate(nullptr, PPD_PROFILE_PERFORMANCE, PPD_PROFILE_ACTIVATION_REASON_USER, &error));
        assert(error);
        for (unsigned policy : policies) assert(read_is(policy, "scaling_governor", "schedutil"));
    }
    g_autoptr(GError) error = nullptr;
    assert(!activate(nullptr, static_cast<PpdProfile>(0), PPD_PROFILE_ACTIVATION_REASON_USER, &error));
    write(base + "3/scaling_available_governors", "powersave schedutil performance-extra\n");
    assert(probe(nullptr) == PPD_PROBE_RESULT_FAIL);
    write(base + "3/scaling_available_governors", "powersave schedutil performance\n");
    write(base + "7/scaling_driver", "other\n");
    assert(probe(nullptr) == PPD_PROBE_RESULT_FAIL);
    write(base + "7/scaling_driver", "qcom-cpufreq-hw\n");
    write("/sys/firmware/devicetree/base/compatible", "xiaomi,other");
    assert(probe(nullptr) == PPD_PROBE_RESULT_FAIL);
    g_print("Uke PPD: exact board/three policies, real governors, write/readback failure rollback and unsupported inputs passed\n");
}
