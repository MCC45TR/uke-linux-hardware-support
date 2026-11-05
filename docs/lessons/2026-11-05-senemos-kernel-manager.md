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

COPR11261914 compiles 1.4.0-1 and passes all six CTest groups. The final
readback review excludes the devfreq zero/no-preference sentinel from offered
values: a driver can report an effective resolved limit instead of the sentinel,
so it is not a verifiable explicit advertised limit. Existing zero-valued state
can still be displayed and handled when validating the other bound. The final
UI preserves fractional MHz and colocates the selected sensor's value/state
with its graph, without changing sampling or hardware policy.

Page-row review found that earlier CPU/sensor pages may remain instantiated in
the navigation stack. Only the current page may open a frequency review or
change the selected graph source; hidden pages must not respond to the shared
manager. A complete-scene test now visits CPU then GPU, prepares one simulated
GPU request, verifies exactly one visible review, proves apply remains blocked
and dismisses the dialog. This protects normal stacked navigation rather than
relying only on standalone controller fixtures.

## Native trial and integrated delivery gate

COPR11261956 (1.4.0-3) passes six groups and was installed through a scoped
Qt-only DNF transaction. Native read-only testing then exposed an asynchronous
first-result race: a pause could invalidate the initial CPU result, and the
future could finish before its queued metadata delivery. The installed suite
reported one failure rather than accepting an empty CPU page. Source2bc1725
retains that already-authorized result when only monitoring is paused, waits
for delivered metadata in live checks and compacts monitoring/source details.
It passes all six groups in COPR11262005. Sourcebc36dfa additionally keeps busy
true until queued delivery and adds an injected-reader regression for the exact
race and paused first read, without sysfs or commands.

The native frequency helper successfully reads the actual Uke provider. The
current PPD performance profile pins every CPU minimum to its maximum. That
profile was preserved: a matching CPU maximum request is a no-op, and lowering
the maximum below the current minimum is refused before writing. The GPU limit
was changed from 950 to 900 MHz, read back, restored to 950 MHz and read back
again. Minimum and governor are identical before and after. These establish
policy-request behavior, not all-OPP clock accuracy or stress qualification.
Automatic staging and rollback preference file hashes remain unchanged.

Concurrent Waydroid GUI work owns the final combined producer and DNF delivery.
The integrated candidate must retain a88d985/76cf43a (current-page reviews),
2bc1725 (initial read and compact graphs), and bc36dfa (queued delivery).
No older partial Qt producer is republished by this conversation. Final combined
build, installation and repeat native read-only checks remain a delivery gate.
No kernel or boot change accompanies this userspace integration.

## Qualified combined result

The final artifact is combined Qt1.5.0-4 from
6748ad4da75d8f344886a9ba2d6c76dfd036b835,
[COPR11262020](https://copr.fedorainfracloud.org/coprs/build/11262020/).
All seven CTest groups pass. The integration owner delivered the signed GUI,
tests and native installer helper with targeted DNF and verified the producer
pin. This conversation did not republish an older partial producer. The manager,
frequency-source/helper and native manager tests compare identically to bc36dfa
in that combined source; the Waydroid feature is retained alongside them.

On the installed combined RPMs, the opt-in read-only scene passes three cases
and populates actual CPU, GPU and sensor pages as well as the existing USB,
charging and NFC pages. The eight native Kernel Manager cases pass, including
the queued-result regression. A separate Turkish/light/Adwaita complete topic
walk passes three cases. Local captures confirm visible CPU frequency/load,
GPU frequency/temperature and selected-source sensor curves. Images and detailed
readings remain private. No empty initial CPU page recurs in this check.

Installed RPM verification is clean. Staging/rollback configuration hashes and
the current PPD performance profile remain identical across the delivery and
checks. Core/client24.speech1 and kernel12101 are unchanged by this work; no
slot selection, reboot, thermal disable or persistent tuning was performed.
GPU remains at its original 950 MHz maximum after the reversible policy test.
CPU mutation under a different power profile and exhaustive all-OPP hardware
clock/stress qualification were not attempted. No new hardware support claim
is inferred from graphs or policy readback.
