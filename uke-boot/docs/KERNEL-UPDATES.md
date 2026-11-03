# Stock-ABL kernel updates

3 November 2026, source 1.8.37 adds continuous trial health qualification, a
three-attempt bootloader budget for new trials, and final fallback readback.
The updater package carries this policy; no new kernel Image or firmware is
required. The first-DNF policy migration introduced in 1.8.36 is retained.
See [userdata rollback](USERDATA-AB.md) for timing, terminal-health handling
and the independent physical acceptance limits.

1 November 2026. Source 1.8.22 runs startup acceptance and policy-gated staging
through one `boot` command under the updater lock. An identical default reuses
the running bundle's verified bytes. Disabled staging still performs health
acceptance; malformed policy and candidate failures retain the existing refusal
behavior. Native checks and physical startup timing remain separate gates.

1 November 2026. Version1.8.21 reads owning RPM metadata and its complete file
inventory in one query per package. Cohort, ownership, module/file digests and
rollback checks remain; malformed or multiple-owner records are refused. Native
Fedora Rawhide AArch64 checks and installed POCO Pad X1 preflight pass. Paired
query measurements improve; complete verifier timing is affected by desktop
load, and a physical startup improvement remains unverified.

31 October 2026. Version1.8.13 enables the guarded kernel transaction by
default. On singleboot userdata installations, DNF writes and verifies the
inactive Linux boot slot, selects it for the next reboot and retains the
accepted running slot and modules for rollback. Existing firmware, power,
identity and transaction checks remain. No automatic reboot is introduced.
Administrator configuration remains preserved by RPM upgrades.

31 October 2026. Updater 1.8.12 reads the complete mount listing with a 1 MiB
bound. A native 4614-byte listing was previously cut at 4096 bytes and refused
as an incomplete row. Strict row/root validation remains; unreadable, oversized
or ambiguous input is refused. COPR tests and native device admission are
separate validation steps. No caller may shorten or filter the mount namespace
to bypass an admission refusal.

31 October 2026, POCO Pad X1, Fedora Rawhide AArch64. Normal DNF update with
updater 1.8.11 replaced an unbooted 7.3.0-60 trial with 7.3.1-3 in slot A.
DNF reported completion before reboot. A normal reboot started 7.3.1-3 and
the packaged health service accepted A. The running Image notes/build ID,
signed package identities, all 821 module digests and native GPT/UFS admission
passed; the complete B/7.3.0-54 image hash remained unchanged. Existing SPMI
and initial-console warnings remain; an automatic suspend attempt failed.
This result does not qualify offline reboot, forced fallback or power loss.

31 October 2026. Updater 1.8.11 releases the old GPT descriptors after a verified
reselection of the accepted slot, before reacquiring fresh metadata to stage a
replacement trial. The global updater lock remains held across both phases.

30 October 2026. Updater 1.8.10 waits up to five seconds for a busy UFS metadata
lock, then refuses the update if exclusivity is still unavailable. Descriptor
identity/capacity failures are reported separately. Native regular-file checks
cover contention that clears, a held lock that times out and invalid descriptors.

30 October 2026. Updater 1.8.9 retains active `transaction BUNDLE` calls from
Uke Control and direct callers. The final RPM trigger also covers profile-only
updates and verifies an already prepared candidate without another image write.
Version 1.8.8's unconditional transaction deferral is superseded.

30 October 2026. Updater 1.8.8 finalizes userdata updates after all RPM package
scripts, including a transaction that changes only the singleboot profile link.
The terminal reports the candidate release, verified target slot and retained
rollback release. A refusal returns a failed scriptlet and says to resolve it
before rebooting; RPM payloads may already be installed.

Normal `sudo dnf update` stages the selected kernel before returning. For an
offline transaction, use `sudo dnf upgrade --offline`, then
`sudo dnf offline reboot`. Boot preparation runs when that stored transaction
executes. An already accepted running baseline is verified without requiring
desktop services in the offline environment. A new trial still requires normal
boot-health acceptance. Physical offline reboot validation remains open.

30 October 2026. Updater 1.8.7 supplies `transaction BUNDLE` for kernel RPM
post-transaction hooks. It honors the selected root profile and administrator
policy. Userdata transactions use the final installed singleboot profile and
the A/B writer. Disabled automatic activation now prints an explicit message;
an installed kernel alone does not change the next boot. See the
[userdata admission check](USERDATA-AB.md) before enabling an installation.

22 October 2026 source update: updater 1.6.0 adds a guarded return from a booted,
unaccepted userdata current-slot trial to its recorded accepted B image. See
[userdata restoration](USERDATA-AB.md#restore-an-unaccepted-current-slot-trial).
It retains the existing package, module, storage, power and write/readback gates.
Host fixtures cover admission and refusal; tablet restoration remains untested.

21 October 2026 correction: updater 1.1.2 also enforces the global disable inside
its native automatic userdata entry point, before boot acceptance or staging.
Both global and userdata flags must be valid and enabled. The explicit manual
commands retain their independent validation. The prior userdata implementation
did not enforce the global flag; its package description was insufficient.

20 October 2026 source update: version 1.1.0 adds a separate guarded
[userdata boot A/B path](USERDATA-AB.md). The Linux-root writer and its explicit
enablement policy remain. The 15 October device results below describe that
earlier Linux-root path; they do not qualify the new userdata implementation.

15 October 2026. Fedora Rawhide AArch64 stock-ABL update implementation.
On Uke, COPR kernel 7.2.9-1.40 and updater 0.1.0-12 passed DNF installation,
complete boot B write/readback and an ordinary reboot to Fedora/KDE. The new
kernel uses an independent module directory and retains the previous tuple.
The firmware framebuffer produced an image; native DPU/DSI remains unqualified.
Early ADSP firmware ordering needs a separate correction before general use.

`uke-kernel-update` accepts an installed package bundle under
`/usr/lib/senemos/kernel-updates/RELEASE`. It checks the exact Uke/SM7675 device,
physical EXT4 root selected by `PARTLABEL=linux`, a unique physical
96 MiB `boot_b`, canonical kernel-only Android v4 payloads, SHA-256 values and
every matching installed module. Android A, GPT, ESP, misc and userdata are
never update targets. The tool does not reboot or select a slot.

Each Linux-only bundle has `linux.img`, `modules.sha256` and a strict
key/value `manifest` containing `format=1`, `kernel_release`,
`linux_image_sha256`, `userdata_image_sha256`, `linux_kernel_sha256`,
`userdata_kernel_sha256` and `modules_sha256`. New root-selecting bundles may
also declare `root_selector_capability=uke-elfnote-v1`; unknown capabilities and
other extra fields are refused. The module inventory contains
SHA-256 rows relative to `/usr/lib/modules/KERNEL_RELEASE`, limited to the exact
`kernel/` module file set. The installed core Image must match the Linux variant;
both userdata manifest values must be `unsupported` for this supported layout.
Payloads must be root-owned and not group/world-writable.

The original update path supports the physical `linux` root. Userdata was not
admitted by that implementation; its new policy and validation gates are
documented separately above.

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

## Legacy package migration

Packages before 1.40 use a shared module directory and broad Obsoletes rules.
They require a bounded metadata migration before coexistence with the new
versioned tuples. Do not remove running modules or globally change installonly
policy as a shortcut. The tested migration preserved every payload byte and
removed only the incompatible package relationships.

The 1.40 source-only COPR build supplies the matched stock-ABL bundle and DNF
hook. Image recipes retain the kernel exclusion until the early ADSP correction
and subsequent update qualification pass. Never write a raw Image to boot B.

Regular-file fixtures cover root-selector refusal, partition ambiguity, module
mismatch, same-release rejection, low/unknown power, malformed boot payloads,
backup/readback and write-error handling. They open no block device. Real
power-loss behavior, repeated upgrades, automatic rollback after faults and
external recovery remain separate validation. The first physical write ran
with SELinux enforcing; exact readback and an idempotent no-write check passed.

The v4 layout follows the [AOSP boot image header specification](https://source.android.com/docs/core/architecture/bootloader/boot-image-header).
Package admission follows the [DNF5 signature configuration](https://dnf5.readthedocs.io/en/latest/dnf5.conf.5.html).

## Matched kernel variants

21 October 2026, updater 1.2.0: package verification admits only the mainline,
mainline-KVM and mainline-unstable source families. Core, modules, DTBs and boot
files must share that family and the exact RPM tuple. KVM and unstable core RPMs
must also provide the exact `kernel-uname-r(...)` capability named by the bundle;
a matching version number in another namespace is insufficient. Rollback module
verification follows the running core's admitted family and checks its RPM files.

This extends package verification without enabling automatic updates. It does
not qualify alternating-slot firmware, suspend, KVM or an unstable kernel boot.
The manual current-slot path retains its physical boot identity, power, durable
backup, readback and protected-partition checks.

## Reuse of the verified running bundle

1 November 2026: when the default bundle is the running kernel, automatic
activation reuses the bytes already verified in that invocation. It retains
the manifest, complete module inventory, RPM ownership, early firmware, physical
boot image and accepted-state checks. The path returns without writing a slot.
A distinct candidate still receives independent candidate and baseline
validation. This removes one repeated bundle validation; device boot timing
and shutdown reliability require separate measurements.

## Native display readiness

1 November 2026: the Plasma Login Manager drop-in retains its bounded udev
wait and allows it to finish when Uke's native display device alias exists.
The original Fedora service and application remain unchanged. Other USB
coldplug work continues independently. The reviewed original service is
`plasma-login-manager-6.8.2-1.fc46`; recheck the drop-in if upstream adds other
pre-start commands. This configuration requires a separate measured reboot
and does not establish physical display or shutdown acceptance.

The packaged `boot` action alone allows a two-second polling window for native
touch calibration during panel handoff. It retries the complete native predicate
every100ms only while an exactly identified, bound input and a uniquely
connected/enabled internal panel remain valid. Ready input or the existing
verified panel-off standby condition is still required; invalid metadata fails
immediately. Explicit acceptance, preflight, legacy touch and transaction paths
keep their immediate checks. The120-second service timeout, five starts and
30-second external restart delay remain. Persistent pending may add two seconds
per attempt; blocked sysfs reads remain subject to the enclosing service timeout.
Native fixture results and actual startup time are separate validation gates.

## Clock initialization and disabled legacy policy

The clock floor comes from the mtime of the packaged `/usr/lib/clock-epoch`,
using systemd's existing epoch handling. Chronyd starts without `-s`: an old
PMIC RTC counter or stale drift-file mtime must not replace this floor before
package verification. Normal NTP correction remains enabled. This does not set
the hardware RTC or claim accurate offline wall time. Acceptance is ordered
after chronyd and `time-set.target`, without an online synchronization dependency.
See the [systemd epoch contract](https://www.freedesktop.org/software/systemd/man/258/systemd.html)
and [chronyd options](https://chrony-project.org/doc/latest/chronyd.html).

Legacy userdata files containing exactly `automatic=0` retain their opt-out
and use no VRR override. A legacy enabled policy still requires an explicit
`vrr_min_hz`; unknown keys and malformed values remain errors. Installing this
update does not enable automatic activation, write an RTC, repair a filesystem
or establish a successful reboot.
# Device-local video firmware

The installation profile also keeps a zero-byte `clock-floor` state file whose
mtime is advanced only while the kernel reports NTP synchronization. Chrony's
profile-owned startup/shutdown drop-in restores that verified lower bound before
Chrony and saves it after shutdown. An admitted new-kernel firmware import must
refresh the floor with NTP synchronization before normal slot staging. The state
is monotonic, root-only and separate from the packaged early epoch; it performs
no RTC access. This covers signatures made later on the same day than RPM's
clamped packaged epoch. Boot and offline-clock acceptance still need device
verification after reboot.

`uke-kernel-update check-oem-video` inspects the physical Uke super partition
read-only and reports a candidate without installing it. The importer admits
mirrored, SHA256-checked Android LP metadata, the measured single-device linear
ODM/vendor layout, and fixed firmware size/hash/ELF memory bounds. Unknown,
changed or absent inputs fail closed. Temporary loop devices are read-only and
autoclear; helper output and runtime are bounded.

`uke-kernel-update import-oem-video RELEASE` additionally requires a fully
validated installed kernel bundle and confines a new firmware file to that
non-running kernel's versioned firmware directory. Publication is atomic and
does not overwrite different existing bytes. Kernel-specific lookup follows
the standard Linux firmware search path. This command neither writes Android
partitions nor activates a kernel or reboots. Firmware bytes are never included
in source archives or public COPR packages. Size/hash admission does not prove
PAS authentication, HFI compatibility or codec operation on hardware.

LP format reference: AOSP system/core commit
`a3b721a32242006b59cb12bd62c9133632af3a2d`,
`fs_mgr/liblp/include/liblp/metadata_format.h` (Apache-2.0).

## One-time image policy migration

3 November 2026, boot integration 1.8.36: the updater's package post-install
migrates the exact legacy image pair `automatic=0` plus LF in both automatic
policy files. Each original file is retained in the protected update state,
then both policies are enabled with a durable migration journal. The legacy
display override remains disabled (`vrr_min_hz=0`). The existing final RPM file
trigger stages the complete selected kernel, verifies the inactive boot slot
and changes the next-boot slot through the existing A/B state machine. There
is no automatic reboot. Installing the new updater also runs that final trigger
against already installed kernel bundles; a new kernel need not be reinstalled.

This is a one-time migration of the old image default. Custom policies are
retained. Once recorded, a later explicit `automatic=0` remains disabled across
upgrades. An interrupted migration can resume only with its admitted original
backups and expected old/new bytes. Unknown, writable, linked or replaced files
are refused. Pending policy writes do not admit boot activation.

Use the normal `sudo dnf update --refresh -y` transaction after the new packages
are published. It installs the updated policy/updater and uses the existing
signature, tuple, firmware, root/UFS, accepted-slot, power and readback checks.
The source update adds no current-slot writer and performs no tablet operation
while building. Source/package fixtures and physical acceptance remain separate.

RPM trigger semantics: [rpm-scriptlets(7)](https://rpm.org/docs/latest/manual/file_triggers.html).

Software validation on 3 November 2026: COPR
[11250994](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11250994/)
completed for Fedora Rawhide AArch64. Native policy/recovery and existing A/B
contracts passed. An isolated RPM upgrade with an already installed boot bundle
recorded migration before exactly one final activation trigger. All six binary
RPMs and the rebuilt source RPM verify with the COPR public key; the delivered
migration hook, final trigger and repository metadata were checked separately.
[Artifact identities](seamless-policy-20261103.json) retain the build source and
digests. No physical slot write, reboot or rollback was performed for this build.
