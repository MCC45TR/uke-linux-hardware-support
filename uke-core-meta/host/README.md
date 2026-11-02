# Retained Core audit and tracing tools

3 November 2026: this COPR recipe adds original Fedora `audit`, `audit-rules`
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
