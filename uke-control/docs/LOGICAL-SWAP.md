# Logical swap in the seed4 installation

Control25 prepares the qualified logical swap before its existing swap selection.
The explicit normal seed4 boot profile is required; legacy boots keep the former
physical-partition or managed-file policy. Invalid opted-in layouts fail before
file allocation. An existing `linux_swap` logical device is admitted only after
the original UFS selector, three ready LP views, owned writable DM table, swap v1
label and complete allocation are independently verified. Foreign mappings,
wrong labels, shorter headers and read-only devices are refused. The mapper
never formats storage or activates swap. Existing Control owns activation and
retires its own old files only after the replacement is active.

The imported shell helpers are pinned to builder commit 90f117a. They use the
original Fedora `android-tools`, `device-mapper`, `util-linux` and core tools;
Android LP metadata is not a Linux kernel partition-table format. The existing
Control package owns this demand-only integration, with no additional runtime
RPM, daemon or polling loop. Original Fedora providers remain unchanged.

The Linux [swap header](https://github.com/torvalds/linux/blob/master/include/linux/swap.h)
and util-linux [mkswap](https://github.com/util-linux/util-linux/blob/master/disk-utils/mkswap.c)
define the version/last-page convention consulted on 16 November 2026. This
installation uses 4096-byte pages and a zero-bad-page v1 header. The mapper does
not claim encrypted swap, safe hibernate restore or authenticated LP metadata.
Those remain separate qualification gates.

The first-setup CDC reboot path now invokes the installed original systemctl
with the fixed bootloader argument. The former call referenced a helper removed
by package consolidation. The four-mode native PTY fixture checks the exact
replacement command and refusal behavior. The retained signed CDC23 checks run
on their exact prepared source view; separate new four-mode checks and the
production binary use the patched tree. The complete recipe and all79 parent
inputs remain preserved by inverse comparison.

Original Fedora owned-loop mapping, negative controls and whole-device byte restoration passed in Actions 43845001508. These build/loop checks do not establish physical swap activation, setup cold boot,
USB reconnect, rescue entry, encryption or camera functionality.
