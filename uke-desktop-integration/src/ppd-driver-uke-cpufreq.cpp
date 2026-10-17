// SPDX-License-Identifier: GPL-3.0-or-later
// CPU governors only: retain firmware OPPs, boost policy and thermal limits.
#include "ppd-driver-uke-cpufreq.h"
extern "C" {
#include "ppd-utils.h"
}
#include <cstring>
#include <initializer_list>

struct _PpdDriverUkeCpufreq { PpdDriverCpu parent_instance; };
G_DEFINE_TYPE(PpdDriverUkeCpufreq, ppd_driver_uke_cpufreq, PPD_TYPE_DRIVER_CPU)
static constexpr unsigned policies[] = {0, 3, 7};

static char *policy_path(unsigned policy, const char *leaf) {
    g_autofree char *path = g_strdup_printf("/sys/devices/system/cpu/cpufreq/policy%u/%s", policy, leaf);
    return ppd_utils_get_sysfs_path(path);
}

static bool read_is(unsigned policy, const char *leaf, const char *expected) {
    g_autofree char *path = policy_path(policy, leaf);
    g_autofree char *value = nullptr;
    return g_file_get_contents(path, &value, nullptr, nullptr) && !strcmp(g_strchomp(value), expected);
}

static PpdProbeResult probe(PpdDriver *) {
    g_autofree char *path = ppd_utils_get_sysfs_path("/sys/firmware/devicetree/base/compatible");
    g_autofree char *compatible = nullptr;
    gsize length = 0;
    if (!g_file_get_contents(path, &compatible, &length, nullptr) || length < sizeof("xiaomi,uke") ||
        memcmp(compatible, "xiaomi,uke", sizeof("xiaomi,uke"))) return PPD_PROBE_RESULT_FAIL;
    for (unsigned policy : policies) {
        if (!read_is(policy, "scaling_driver", "qcom-cpufreq-hw")) return PPD_PROBE_RESULT_FAIL;
        g_autofree char *available_path = policy_path(policy, "scaling_available_governors");
        g_autofree char *available = nullptr;
        if (!g_file_get_contents(available_path, &available, nullptr, nullptr)) return PPD_PROBE_RESULT_FAIL;
        g_auto(GStrv) governors = g_strsplit_set(available, " \t\n", -1);
        for (const char *name : {"powersave", "schedutil", "performance"})
            if (!g_strv_contains(governors, name)) return PPD_PROBE_RESULT_FAIL;
    }
    return PPD_PROBE_RESULT_SUCCESS;
}

static gboolean activate(PpdDriver *, PpdProfile profile, PpdProfileActivationReason, GError **error) {
    const char *governor;
    switch (profile) {
    case PPD_PROFILE_POWER_SAVER: governor = "powersave"; break;
    case PPD_PROFILE_BALANCED: governor = "schedutil"; break;
    case PPD_PROFILE_PERFORMANCE: governor = "performance"; break;
    default:
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Unsupported CPU profile");
        return FALSE;
    }
    char *previous[3] = {};
    bool success = true;
    unsigned attempted = 0;
    for (unsigned i = 0; i < 3; i++) {
        g_autofree char *path = policy_path(policies[i], "scaling_governor");
        if (!g_file_get_contents(path, &previous[i], nullptr, error)) { success = false; break; }
        g_strchomp(previous[i]);
    }
    if (success) for (unsigned i = 0; i < 3; i++) {
        g_autofree char *path = policy_path(policies[i], "scaling_governor");
        attempted = i + 1;
        if (!ppd_utils_write(path, governor, error)) { success = false; break; }
        if (!read_is(policies[i], "scaling_governor", governor)) {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "CPU governor readback failed");
            success = false; break;
        }
    }
    if (!success) for (unsigned i = 0; i < attempted; i++) {
        g_autofree char *path = policy_path(policies[i], "scaling_governor");
        g_autoptr(GError) rollback_error = nullptr;
        if (!ppd_utils_write(path, previous[i], &rollback_error) ||
            !read_is(policies[i], "scaling_governor", previous[i])) {
            g_prefix_error(error, "CPU policy%u rollback failed; ", policies[i]);
        }
    }
    for (auto value : previous) g_free(value);
    return success;
}

static GObject *constructor(GType type, guint count, GObjectConstructParam *params) {
    auto object = G_OBJECT_CLASS(ppd_driver_uke_cpufreq_parent_class)->constructor(type, count, params);
    g_object_set(object, "driver-name", "uke-cpufreq", "profiles",
                 PPD_PROFILE_POWER_SAVER | PPD_PROFILE_BALANCED | PPD_PROFILE_PERFORMANCE, nullptr);
    return object;
}

static void ppd_driver_uke_cpufreq_class_init(PpdDriverUkeCpufreqClass *klass) {
    G_OBJECT_CLASS(klass)->constructor = constructor;
    auto driver_class = PPD_DRIVER_CLASS(klass);
    driver_class->probe = probe;
    driver_class->activate_profile = activate;
}

static void ppd_driver_uke_cpufreq_init(PpdDriverUkeCpufreq *) {}
