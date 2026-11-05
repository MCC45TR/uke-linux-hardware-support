# UKE-KERNEL-RESOURCES-20261105: derive controls and graphs from kernel providers

Date: 2026-11-05. Environment: Fedora Rawhide AArch64, POCO Pad X1,
running 7.3.1-12203-unstable. Source baseline: combined Qt 1.5.0-5 with the
qualified Waydroid integration dependency. Native compilation is COPR only.

The running kernel exports CPU frequency residency, idle counters, PSI,
vmstat, block queue statistics, thermal zones and cgroup CPU weights/memory
limits. It lacks CPU bandwidth, uclamp, zram and MGLRU support. Root storage
is a partition whose scheduler belongs to its parent queue. UFS devfreq is
not GPU telemetry. A charge-threshold file can exist and return ENODATA.

The resource reader keeps absent or denied data distinct from numeric zero.
Rates require two monotonic observations without a counter reset; disk sectors
are 512 bytes independently of queue logical block size. Mean completion time
requires completed operations and is not a percentile. CPU die aggregation
excludes the board CPU thermistor. Fixed-ID runtime writes check reviewed
state and canonical provider and read back the result. Application changes
are restricted to the current user's active app unit and invocation.

Native parser/ownership/refusal fixtures and the retained Qt/Waydroid suite
are queued for COPR validation. This source record does not claim build,
installation, kernel feature or physical hardware acceptance. Kernel feature
integration remains with the shared coordinator; existing source/config,
firmware, camera/audio and rollback gates must be retained.
