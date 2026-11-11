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
