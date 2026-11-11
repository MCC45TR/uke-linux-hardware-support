# Runtime package retirement

11 November 2026, Fedora46/AArch64 on Uke kernel7.3.3-23-unstable.

Control2.17.0-2 owns the speaker executable and provisioning unit, providing
`uke-speaker-runtime`0.4-4 and `gain-readiness-v1`. The former standalone0.4-3
package is already absent from the tablet. A fresh DNF solver run excluding
that package resolves the current Control/TUI pair. The accepted2.16.0-2
rollback resolves with the archived charge-control1.4.1-7 package, preserving
the speaker provider inside Control. Neither preview was applied. An older
2.13 rollback preview removed unrelated packages and was rejected.

The exact speaker0.4-3 source and signed runtime are preserved in the existing
[consolidation recovery release](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101),
with a separate `SPEAKER-CONTROL-FOLD-SHA256SUMS-20261111` manifest. Public
readback matched both hashes. The Control source constructor consumes the
archived SRPM with its unchanged pinned hash. No live producer references the
old build URL. Current kernel sources have no standalone speaker constructor.
The family retirement guard prevents recreating the redundant runtime RPM.

The two installed test packages, `uke-boot-graph-audit-unstable`0.1-3 and
`uke-control-gui-qt-tests`1.6.0-6, were removed through DNF without removing
other dependencies. Boot policy, kernel23/A, rollback21/B and runtime apps
were preserved. GUI1.6.0-7 retains all native CTest checks while dropping
installed test/debug RPMs; its build and repository cleanup are separate gates.

These ownership and solver results do not qualify physical audio, gain243,
stationary closed-cover startup or native sensor heading. Gain223 remains.

Control217r3/COPR11308566 was subsequently installed through the configured
repository. All52 parent source files,197 data files,32 ELF interfaces/libraries/
loadable code sections,SELinux CIL and all file ownership/flags are retained.
Observer/offline services retain their inactive disabled/static states. Fresh,
reinstall and archived217r2/battery7 rollback resolve. Firmware15/COPR11309337
also retains all227 parent data/license files and the generic preset's noreplace
flag;isolated signed DNF upgrade/rollback preserves a synthetic local edit.
Actual device upgrade/reinstall/archive rollback and raw boot checks pass.

The battery and speaker-firmware producers3272002/3266596 are retired with
exact GET404 and refreshed standalone-output absence. Existing Control and
firmware producers use qualified source workers with successful-NVR refusal.
A fresh Core21/Control217r3/firmware15 solve excludes all five obsolete provider
names. Hardware source contracts43081867981 and the complete30-producer/
31-installed-name inventory pass without unknown entries. Device names fell
36 to31;producer definitions33 to30. Required original-provider and hardware
parity work remains open. Current kernel23/A,rollback21/B and gain223 remain.
