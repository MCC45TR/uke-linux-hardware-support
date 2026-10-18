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

The system-wide record is `/var/lib/senemos/installation/profile`, owned by
root with mode `0600`, inside a `0700` directory. For example:

```ini
format=1
root=userdata
```

After the atomic, synchronized write, the classifier creates
`/etc/systemd/system/uke-install-profile.service -> /dev/null`. This persistent
systemd mask prevents subsequent starts across boots and package upgrades.
The unit's missing-profile condition also prevents repeated classification
if power fails between the profile write and masking. Neither mechanism uses
a home directory, login account or per-user service. A Linux profile also
masks the two planned userdata units. No periodic task or resident classifier
is installed. The package remains inactive until an image explicitly enables
the first-boot unit.

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

The revised package plan uses one `senemos-uke-linux-kernel-mainline` family,
one Image and matching modules. A kernel candidate admits only the boot-header
selector `senemos.root=linux|userdata`, while preserving the forced command
line for every other option. The transaction helper will select the boot
profile from installation state and revalidate the mounted physical root
before writing. DNF does not need to modify GPT entries or filesystem labels.
No `-up` package is needed if the Uke bootloader handoff is qualified. The
current updater still refuses userdata; no new writer is activated here.

Remaining work is the qualified Qualcomm slot backend, durable update
transactions, common-kernel/bootloader qualification, transaction dispatch,
inactive-slot boot writes, userdata-aware startup readiness, bounded success
confirmation and a recovery installer. ZIP and test-image creation are deferred
until the requested source revisions are complete. Bootloader retry and watchdog
behavior need separate Uke evidence. Kernel/boot A/B with a shared filesystem
does not roll back general DNF packages or user data.

The implementation sequence and source comparisons are tracked in the
[private engineering plan](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/testing/USERDATA-AB-UPDATE-PLAN-2026-10-18.md).
