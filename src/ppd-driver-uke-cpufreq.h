/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <glib-object.h>
G_BEGIN_DECLS
#include "ppd-driver-cpu.h"
#define PPD_TYPE_DRIVER_UKE_CPUFREQ (ppd_driver_uke_cpufreq_get_type())
G_DECLARE_FINAL_TYPE(PpdDriverUkeCpufreq, ppd_driver_uke_cpufreq, PPD, DRIVER_UKE_CPUFREQ, PpdDriverCpu)
G_END_DECLS
