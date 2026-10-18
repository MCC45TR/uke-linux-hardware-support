# Userdata A/B update preparation

18 October 2026. Fedora Rawhide AArch64 source and host-test implementation.
This work does not yet provide an alternating-slot updater or a recovery
installation image. The existing Linux-root boot-B update path is unchanged.

The optional `uke-installation-profile` RPM supplies a C++ executable and an
inactive first-boot oneshot. It opens no block device. The composer must
explicitly enable the unit in a future qualified image.

`uke-install-profile check` inspects the mounted physical Uke EXT4 root and its
matching `root=PARTLABEL=linux` or `root=PARTLABEL=userdata` selector without
writing. `initialize` creates a root-owned system profile under
`/var/lib/senemos/installation/`, then masks its first-boot unit. Linux-root
installations also mask `uke-userdata-update.service` and
`uke-userdata-accept.service`. Those planned services are not provided yet.
`status` reads the stored profile without probing the root partition again.

Profile writes use descriptor-relative protected paths, atomic replacement,
file and directory fsync, and a process-wide file lock. Unknown roots produce
a disabled diagnostic profile; they never activate an updater. Administrative
unit overrides are preserved and reported for review. To retry a failed
classification, an administrator must first resolve the recorded problem and
explicitly remove the diagnostic profile and unmask the classifier. Do not
reuse profiles from a different installed root or include consumed first-boot
state in exported images.

The independent C++ policy model checks A-to-B and B-to-A staging, verification
before selection, matching trial acceptance, fallback and refusal to
automatically restage the same failed release. A pending transaction blocks a
second staging request. This policy model performs no slot I/O and is not a
bootloader rollback implementation.

Run `make test-installation` for regular-file and policy tests. Set
`SANITIZE=1` when the host compiler has matching ASan/UBSan runtimes. These
checks do not open tablet block devices or establish physical boot success.

Remaining work is the qualified Qualcomm slot backend, durable update
transactions, mutually exclusive standard/`-up` kernel selections from one
source build, inactive-slot boot writes, bounded success confirmation, recovery
installer and checksummed userdata test image. Bootloader retry and watchdog
behavior need separate Uke evidence. Kernel/boot A/B with a shared filesystem
does not roll back general DNF packages or user data.

The implementation sequence and source comparisons are tracked in the
[private engineering plan](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/testing/USERDATA-AB-UPDATE-PLAN-2026-10-18.md).
