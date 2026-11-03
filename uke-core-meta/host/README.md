# Retained Core audit and tracing tools

3 November 2026: COPR build 11247345 passed the retained metadata/XML and
native QML/cache checks on Fedora Rawhide AArch64. It emitted only Core19.1
and the source RPM. Full signatures passed; Core file contents, permissions,
ownership and lifecycle scripts match the installed19. The only added runtime
requirements are original Fedora audit collection and tracing tools.

The exact DNF update installed Core19.1, audit/audit-rules4.2.1-2 and strace7.3-1
on Fedora46. Targeted RPM verification passed. Auditd is active/enabled with
zero lost/backlog counters and enforcing SELinux; default rotation is8MiB
with5files. Fedora's default task-never rule retains hardwired-event collection
while syscall auditing remains off. This does not recover past missing records
or establish complete security coverage.

This COPR recipe adds original Fedora `audit`, `audit-rules`
and `strace` to the existing Core selection. The reviewed device had no audit
collector or syscall tracing tool; kernel audit-print suppression limited its
log investigation. No project daemon, new package name, backend or SDK is added.

The source retains signed Control build 11240837 and emits only
`uke-core-meta-1.0.7-19.1.fc46`, using source `uke-control-2.8.1-5.2.fc46`.
It preserves the Core payload and lifecycle/file sections, runs the original
metadata checks, then prunes unselected buildroot files. Both revisions remain
below the separate backend/SDK cohort's next integer releases.

Run the recipe only in COPR `mcc45tr/uke-linux-unstable`, chroot
`fedora-rawhide-aarch64`, at an immutable reviewed Git revision. Use a one-time
custom build without replacing the Control producer. Inspect the actual sole
binary, signatures, file digests and runtime dependency diff before the exact
DNF update. Separately verify audit collection, bounded log rotation and a
short private tracing window. Do not infer those runtime results from source
or build checks; no kernel activation or reboot is requested by this recipe.

Retain these original Fedora dependencies in future Core selection revisions.
Tested source: `4ce65b72eb41bc36cef62707b03192afd69b7ade`. Source5.2 and
Core19.1 are reserved; future changed artifacts must advance their releases.
