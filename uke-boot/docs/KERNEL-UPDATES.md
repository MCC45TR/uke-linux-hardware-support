# Stock-ABL kernel updates

15 October 2026. Fedora Rawhide AArch64 stock-ABL update implementation.
Package builds, regular-file tests and physical update tests are recorded
separately. Physical DNF installation and first reboot remain unqualified.

`uke-kernel-update` accepts an installed package bundle under
`/usr/lib/senemos/kernel-updates/RELEASE`. It checks the exact Uke/SM7675 device,
physical EXT4 root selected by `PARTLABEL=linux`, a unique physical
96 MiB `boot_b`, canonical kernel-only Android v4 payloads, SHA-256 values and
every matching installed module. Android A, GPT, ESP, misc and userdata are
never update targets. The tool does not reboot or select a slot.

Each bundle has `linux.img`, `userdata.img`, `modules.sha256` and a strict
key/value `manifest` containing `format=1`, `kernel_release`,
`linux_image_sha256`, `userdata_image_sha256`, `linux_kernel_sha256`,
`userdata_kernel_sha256` and `modules_sha256`. The module inventory contains
SHA-256 rows relative to `/usr/lib/modules/KERNEL_RELEASE`, limited to the exact
`kernel/` module file set. The installed core Image must match the Linux variant;
the userdata variant must come from the same separately validated module ABI.
Payloads must be root-owned and not group/world-writable.

The first update path supports the physical `linux` root. A `userdata` boot
variant is not admitted by this writer; publishing a separate preview image
does not qualify in-system updates for that layout.

The new kernel must have a **different kernel release**, and running-kernel
modules must remain installed. A package revision that replaces the same module
directory is refused. Verified battery capacity must be at least 50%, or at
least 30% with external power online. Missing or invalid capacity is refused.
Before writing, the current boot
must match the running release and have compatible OS metadata. The updater
syncs a complete private backup, writes the candidate, syncs and reads it back.
A write/readback error attempts a synced restoration. If storage itself fails,
restoration can fail too; the retained backup and external recovery route remain
necessary. A single-slot write is not power-loss atomic.

## DNF integration

The updater package installs `/etc/senemos/kernel-update.conf` with
`automatic=0`. Installing it does not write a partition. A matching kernel
`-boot` RPM uses its post-transaction hook only when this file contains the exact
line `automatic=1`. This gate keeps image composition and other ARM64 systems
outside the writer. The native command checks device and storage independently.

Boot bundles must be owned by `senemos-uke-linux-kernel-mainline-boot`, with the
same RPM epoch, version, release and AArch64 architecture as the installed core,
modules and DTB packages. The writer checks the relevant installed RPM file
digests in addition to the bundle inventory. Signature verification belongs to
DNF's package admission: keep the COPR repository's package GPG checking enabled.
Do not install untrusted RPMs or disable signature checks. The local RPM database
and root administrator are trust boundaries, not independent signature proof.

After a matching boot bundle has passed `uke-kernel-update check`, explicitly
enable its transaction hook in `kernel-update.conf`. Subsequent matching kernel
transactions can update `boot_b` while Fedora remains running. Reboot remains
manual. If a post-transaction check refuses the write, the RPMs can already be
installed while the previous boot remains selected; read the reported error,
correct its cause, then retry `uke-kernel-update apply` with the installed bundle
path before rebooting. A successful RPM transaction alone does not prove the
boot partition changed or the new kernel started.

## Legacy package limit

The existing COPR kernel provides a raw Image and uses a shared module directory
for package revisions. It does not yet provide these stock-ABL bundles. This
helper therefore **does not make legacy `dnf upgrade` update boot_b**. Image
recipes must keep the kernel exclusion until versioned kernel/module packaging,
complete public boot payload generation and the DNF transaction hook are ready.
Do not add a hook that writes the raw Image or remove that exclusion early.

Regular-file fixtures cover root-selector refusal, partition ambiguity, module
mismatch, same-release rejection, low/unknown power, malformed boot payloads,
backup/readback and write-error handling. They open no block device. Real
storage/power-loss behavior, SELinux permission for the writer, DNF upgrade and
rollback, first reboot and recovery acceptance remain separate validation.

The v4 layout follows the [AOSP boot image header specification](https://source.android.com/docs/core/architecture/bootloader/boot-image-header).
Package admission follows the [DNF5 signature configuration](https://dnf5.readthedocs.io/en/latest/dnf5.conf.5.html).
