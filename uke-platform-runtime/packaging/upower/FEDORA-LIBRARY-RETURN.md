# Return UPower libraries to Fedora

12 November 2026. Candidate1.91.5-21 retains the accepted daemon's actual20
source archive and all five patches. It changes library/package ownership;
it does not switch to the original Fedora daemon, qualify charging actuation or retire the
remaining daemon exception.

The original `upower-libs-1.91.5-1.fc46.aarch64` has the same SONAME, symlink,
197 exported ABI records, runtime requirements and byte-identical
UPowerGlib-1.0 typelib as installed Uke20. ELF files have different hashes;
byte-identical executable code is not claimed. A private-bus/read-only-sysfs
kernel24 trial runs the installed daemon with its current library, with the
original library, and the original Fedora daemon/library pair. Nine selected
battery properties match in that normal current-boot observation. Production
daemon PID/restarts/configuration are retained and no charging SET is possible
through the read-only sysfs. History stays in private noswap RAM.

The first guards reject duplicate mountpoint records before starting a daemon.
The final guard opens `/sys`, reads that descriptor's kernel mount ID and checks
the resolved mount's read-only flag. A shadowed writable mount is distinct from
the visible read-only bind. This is an observation correction, not a kernel or
UPower defect. Normal telemetry does not close previously reproduced unknown
health and late-capacity failures or the required-write error contract.

## Necessary daemon exception

The remaining production delta is required-write error propagation, reported
health versus design estimates, standard Wireless diagnostics and late
capacity/unit recovery. The fifth patch controls only the uninstalled history
self-test's clock. These changes do not affect the client library. They are
documented in the canonical native-battery/original-UPower assessment.

Hardware protocol, data validity and lifecycle stay in kernel/DT. The daemon's
unit discovery, health estimation and D-Bus error handling are userspace
behavior. Hiding valid battery information or fabricating values in the kernel
to avoid those defects would not establish feature parity. Return the daemon
to the original Fedora package after its affected error/late-data/health cases,
settings/history and supported DNF rollback are qualified. Do not keep a library
fork for daemon-only changes.

## Candidate package transition

The daemon requires the original Fedora library and obsoletes/conflicts with
the retired Uke library name. The existing development and documentation names
remain only as empty metadata bridges requiring their original Fedora packages.
They own no headers, typelib, library, API XML or documentation. Their temporary
role preserves existing development installations during ordinary DNF updates;
no new runtime package, daemon or polling helper is introduced.

The recipe reverses completely to the actual accepted20 recipe outside these
declared changes. Its source archive remains byte-exact. Existing threshold,
health, Wireless, late-capacity and history regressions remain, now using the
staged daemon and original Fedora library. Native COPR, signed payload/ownership,
fresh runtime/development plans, actual upgrade/reinstall/rollback and current
service/preference readback are required before delivery or retirement. This
document and the ABI/private-bus trial do not satisfy those later gates.

Source baseline: original UPower1.91.5 commit
`233615a1505166ef4a28c959836fe8c7781f5697` (GPL-2.0-or-later), Fedora
dist-git `f47a2ad5d41e310ae0fb6e23ddcce78ae4ae9c87`, accepted build11228197.
The actual parent SRPM/source/spec identities are pinned in `fedora-libs.json`.
