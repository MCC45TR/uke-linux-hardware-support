# UKE-KERNEL-MANAGER-20261105: colocate kernel observations and frequency controls

Date: 5 November 2026. Environment: POCO Pad X1, Fedora Rawhide AArch64,
7.3.1-12101-unstable; core/client 2.8.1-24.speech1 and prior Qt 1.3.0-6.

## Evidence and source comparison

The running kernel exports CPUFreq policies for CPUs 0–2, 3–6 and 7 through
qcom-cpufreq-hw. Their hardware tables top out at 1.9008, 2.6112 and 2.8032 GHz.
The adreno-bound GPU exports devfreq frequencies from 255 to 950 MHz. The
similarly named UFS devfreq device is storage, not a GPU. Standard GPU load
percentage is absent. CPU cpuinfo_cur_freq is privileged, whereas the driver's
scaling_cur_freq is readable. Frequency must not become a utilization estimate
or a claim of an instantaneous hardware clock.

The donor review starts with the exact Uke OEM kernel
5712080a4b0c0b1a879e6d145c601c649ddaddfd,
`drivers/cpufreq/qcom-cpufreq-hw.c`, and the pinned Uke source/DT survey in
`uke-linux-docs/docs/research/MICODE-UKE-SOURCE-SURVEY-2026-10-15.md`.
The CPU hardware LUT and runtime-advertised policies remain authoritative.
The shared-IP comparison is the standard upstream CPUFreq/devfreq interface,
not another board's OPP table. No donor code, voltage table, Android policy
hook, thermal threshold or kernel patch is imported. Relevant primary ABI:
[CPUFreq](https://docs.kernel.org/admin-guide/pm/cpufreq.html) and
[devfreq](https://raw.githubusercontent.com/torvalds/linux/master/Documentation/ABI/testing/sysfs-class-devfreq).

The existing native sensor owner returns 195 observations in this trial,
including kernel thermal/hwmon/IIO channels and cached SensorProxy values.
Its unit conversion and claim lifecycle are reused. Missing values, inaccessible
input switches and absent samples remain unknown; repeated cached values do not
establish fresh acquisition or calibration. The current snapshot contains 51
thermal zones. Detailed sensor observations remain private.

## Implementation

Senemos Kernel Manager groups CPU, GPU, sensors and existing kernel/slot
management. CPU/GPU pages colocate graphs, current limits/governor, driver facts
and reviewed controls. Memory/swap and sleep/wake preferences are also colocated
with their observations; expert SDK actions remain accessible.

A native bounded worker reads frequency, thermal and proc CPU counters without
blocking QML. CPU load uses counter differences and excludes idle/iowait; first,
missing and reset counters do not become zero. Session graphs use monotonic host
observation times, bounded 600-point rings, explicit gaps and pointer/touch
inspection. Read intervals are 1, 2 or 5 seconds. Monitoring pauses off-page or
when the application becomes inactive and stops after ten minutes. No recording
service or file history is introduced. Sensors use the existing bounded SDK
one-shot read; the source's unknown sample age is shown alongside values.

A separate narrow PolicyKit helper changes one standard attribute per request.
It verifies Uke identity, provider, advertised frequency/governor, hardware and
min/max bounds, expected current configuration, sysfs filesystem, fixed attribute
names and readback. The GPU selector admits only adreno-bound devfreq objects.
Userspace governors are excluded because this UI does not own a setspeed loop.
Changes are not saved for boot; kernel thermal/firmware controls remain intact.
There is no arbitrary sysfs writer, overclocking, voltage tuning or kernel/profile
selection. A setting changed by another owner invalidates the reviewed request.

## Validation

COPR compilation, fixture results, installed queries and actual policy writes
will be recorded separately. Native compilation and Python do not run on the
tablet. Frequency tests use injected I/O and cover bounds, stale state, one-write
behavior, no-op handling and mismatched readback. Graph tests cover counter
validity, bounded history, missing sensors, simulation isolation and sample
inspection. Physical tuning must use a small reversible limit change followed
by exact restoration; it must not change governors or unrelated policies by
assumption. No stress load, voltage, thermal-disable, reboot or boot write is
required. Automatic staging and the existing rollback-off preference are kept.
All kernel contributions and deliveries remain with the shared coordinator.
