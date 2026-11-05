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

Follow-up validation: COPR11263129/Qt1.6.0-1 passed all8 native test groups;
the3 userspace RPMs were installed through DNF and verified with rpm-V.
Installed regression checks passed. Actual memory/swap/PSI and root sda queue
reads succeeded. A transient swappiness60->61 request read back61; a stale
expected60 request was refused without writing; restoration to60 passed.
A dedicated user test service accepted CPUWeight100->25->100 through the
application helper and was stopped afterward. CPU quota was accurately absent.
No kernel, boot slot or desktop power profile was changed by these checks.

COPR11263183/Qt1.6.0-2 compiled successfully and passed7/8 groups. The richer
QML fixture correctly exposed undefined integer bounds in hidden categorical
editors; the source now initializes those editors without weakening warning
checks. The running follow-up11263202 remains an independent trial. Final
qualification requires the corrected package's whole retained suite and actual
read-only resource scenes. DRM client availability and kernel-only experiments
remain separate from GUI/source acceptance.
