# Boot layout conversion

25 October 2026. Control 2.1.1 Uke Fedora source implementation. Source, disposable VM tests,
package composition and physical tablet acceptance are separate results.

Run `sudo uke-control multiboot` or `sudo uke-control singleboot` in a terminal.
The original distribution `dialog` supplies keyboard navigation and mouse support.
`--dry-run` runs the selection and review steps and stores private metadata on the
selected external journal; it does not prepare target writes or reboot.
`singleboot status` and `multiboot status` are read-only hardware admission queries.
Use explicit `--json` for machine-readable status.

1 November 2026, Control 2.8.1: status also lists `offline_blockers`.
An empty list covers the observed watchdog requirement only; original GPT,
ownership, external journal, backup and the exact plan remain separate checks.
A missing watchdog state is blocked explicitly. Boot a kernel built with
`CONFIG_WATCHDOG_SYSFS=y`; an active or unknown watchdog remains blocked.
Reading status never opens a watchdog device, prepares shutdown or resizes F2FS.

For retained recovery-created layouts, the native helper can import a completed
`ure-adb-dualboot-checkpoint` into a new private external metadata directory:

```sh
sudo /usr/libexec/uke-control/layout-manager --import-recovery \
  /external/private/transport.json /external/private/imported
```

The input must be a root-owned mode-0600 copy of the retained final checkpoint.
The helper verifies its complete twelve-member snapshot, sealed plan and terminal
state, unit/LUN/disk identity, healthy original GPT and byte-exact current GPT.
It retains the checkpoint and creates `original-gpt/` plus `ownership.json` for
the wizard. It does not infer ownership from labels, waive an unknown partition,
write the target disk, format a filesystem or prepare a reboot. Keep the original
recovery archive independently. Self-checksums bind bytes, not an OEM signature;
the supplied checkpoint must come from trusted retained creation evidence.

The wizard offers ESP, Linux boot, Linux root, swap, Windows, shared data and a
second Linux root. Sizes accept MB, MiB, GB, GiB and percentages of the original
userdata pool. Advanced mode permits custom labels and role order. Userdata must
remain first, at its original start and GUID; every child follows it. ESP, Windows
and swap retain their required filesystem types. Native format/check tools must
be available; unavailable adapters and inadequate format sizes block the plan.
ZFS has no admitted pool adapter and is unavailable.

```text
singleboot / multiboot
├─ Identify exact Uke / SM7675 and the unique writable 4 KiB UFS userdata LUN
│  └─ Missing identity, ambiguous devices or unhealthy GPT → refuse
├─ Verify durable private journal on a separate direct ext4/XFS or single-device Btrfs device
├─ Establish original same-unit/LUN/disk GPT and original userdata extent
│  ├─ Uncarved layout → capture, synchronize and verify both GPT copies
│  └─ Existing children → require original GPT and completed exact ownership receipt
│     └─ Name, location or an incomplete receipt alone → refuse
├─ Build layout strictly inside the original userdata extent
│  ├─ Singleboot → remove only proven children; restore full original userdata range
│  └─ Multiboot → userdata first; selected children immediately after it
├─ Compare protected OEM records byte for byte in both GPT tables
│  └─ Preserve MBR, disk GUID, metadata locations and usable bounds
├─ Select file policy and execution route
│  ├─ Preserve → verify filesystem, UUID, size, single-device layout and internal journal
│  │  ├─ Mounted Ext4 shrink or F2FS growth → shutdown route
│  │  ├─ Mounted Btrfs shrink or Ext4/Btrfs/XFS growth → native live operation
│  │  ├─ F2FS/XFS shrink → block preservation; explicit recreation needs two approvals
│  │  └─ Unsupported preservation → block; never silently erase
│  ├─ Erase userdata or existing children → two new plan-bound approvals
│  └─ Active affected swap/holders, removed root or formatted mounted root → shutdown
├─ Revalidate exact consent pinned by the terminal, plan, device and original proof
├─ Execute once with an external phase journal
│  ├─ Synchronize verified before/after GPT archives before filesystem changes
│  ├─ Shrink filesystem first; prove it fits the proposed userdata boundary
│  ├─ Direct route → update kernel partition ranges before GPT commit
│  │  └─ Kernel refusal → undo only completed kernel changes while GPT is still old
│  ├─ Commit backup GPT, then primary GPT; synchronize and read back each copy
│  │  └─ I/O error → stop writes; classify observed GPT, never blindly restore it
│  ├─ Grow preserved filesystem only after its partition has grown
│  └─ Initialize selected volumes through individually bounded device-mapper views
├─ Verify both GPT copies, OEM records, filesystem geometry and preserved UUID
│  ├─ Require preserved filesystem capacity within 16 MiB of the requested boundary
│  └─ Record completed ownership only after all checks succeed
└─ Finish
   ├─ Direct route → remain in Linux or request normal bootloader restart
   └─ Shutdown route → RAM worker after oldroot unmount, then restart to fastboot
      └─ Failure → preserve journal, no automatic retry, bootloader or poweroff
```

The planner and original-GPT verifier reuse unchanged project-owned OrangeFox
sources at `8395560393f78bcbe2adeaedc44057c0aa7141de`. The source receipt is in
`shared/recovery/source.json`; Apache-2.0 attribution is retained. The recovery
live-write guard is unchanged, and its general write executors are not linked.
The Fedora executor only writes reviewed GPT metadata and partition views inside
the proven userdata pool. OEM payload nodes are never passed to a formatter.

Preservation supports single-device Ext4, F2FS, Btrfs and XFS with verified
geometry. F2FS and XFS shrink are unavailable. A filesystem already small enough can fit a
smaller partition without a filesystem shrink. Kernel or checker failure stops
the operation; it never chooses destructive recreation. Offline checks do not
repair corruption automatically. NTFS checking uses `ntfsfix`, which is not a
replacement for Windows `chkdsk`. External filesystem journals, multi-device
filesystems and ambiguous signatures block both preservation and recreation;
additional confirmations cannot waive the userdata-only boundary.

F2FS preservation shrink is disabled following a qualification failure: a large
online shrink returned success, but the subsequent read-only checker reported
SIT/current-segment inconsistencies. This was observed with the generic Fedora
7.4 VM kernel and f2fs-tools 1.16.0-11.fc45, not on Uke. The kernel/tool cause
remains unresolved. No size-based exception or automatic repair is admitted.
Native ELF validation preserves the requested executable alias: Fedora's
`resize.f2fs` uses the same ELF as `fsck.f2fs` and selects its action from argv[0].
Final capacity is checked independently; a zero tool exit status alone does not
establish successful growth.
Mounted XFS geometry is read through XFS_IOC_FSGEOMETRY_V1 on a mount whose
device number matches the admitted partition. Its UUID, data-block count and
internal-log/no-realtime boundary are checked. A live XFS primary superblock
observation can lag the mounted geometry and is not used as the sole size proof.
The distribution xfsprogs-devel UAPI headers are a build dependency.

An external root-private directory on an independent ext4/XFS or single-device
Btrfs disk is mandatory. Btrfs journals must use the top-level filesystem root;
subvolume/bind mounts, multiple devices and virtual backing remain blocked.
Backing-device identity and filesystem UUID are verified before and after the
shutdown remount. Btrfs remount explicitly selects subvolid=5.
It holds original GPT proof, the sealed plan, approvals, verified before/after
GPT archives, operation phases and the
completed child-ownership receipt. Keep that directory for future conversion.
Existing layouts without suitable provenance are refused, including layouts whose
names merely resemble Uke roles. Metadata backups do not contain user files.

Shutdown work uses a temporary audited dracut RAM environment, a current-boot
approval with a ten-minute preparation lease measured with CLOCK_BOOTTIME,
existing kernel filesystem ioctls
and the bound nvmem bootloader restart interface. It adds no permanent startup
service or kernel workqueue. An existing shutdown environment is preserved. An
active watchdog without an admitted RAM keeper blocks this route. The shutdown
worker independently revalidates the plan after oldroot is unmounted. It never
continues a partially started job automatically. The RAM request pins the exact
renewed consent fingerprint; changing and rehashing external records cannot
replace the approved operation. A refused reboot request
withdraws the prepared task.

A transient systemd worker and a shutdown/sleep inhibitor protect active work
from terminal closure and ordinary shutdown. Critical storage tools are waited
for without a destructive timeout. Power loss and hardware I/O faults remain
possible; maintain a file backup and recovery route. Metadata archives are
recovery evidence, not an automatic undo command. After a failure, keep the
device stopped and retain the external journal. Inspect both GPT copies and
`state.json` before selecting a recovery layout. An old table must not be restored
over a grown or replaced filesystem; `payload_replacement_or_growth_started`
marks this boundary conservatively before the tool runs. A successful shrink
may also have changed filesystem metadata while GPT still matches the before
archive. Erased files cannot be restored from GPT archives. Restoring metadata
requires a separately reviewed recovery operation.

The manager does not install an operating system, populate an ESP, recreate
Android encryption keys, or change OEM boot/firmware/slot records. Its completion
instructions identify the system-image destination and require a matching boot
image root contract. A layout containing a replaced running root must finish in
fastboot, where the operator flashes the appropriate system image.

Host-only checks: `make check-layout`. Disposable guest integration checks:
`bash tests/layout-vm.sh`. The VM fixture is excluded from installed RPMs and
has no production device-selection override. Physical Uke conversion, shutdown
handoff, fastboot entry and file preservation still require tablet acceptance.

The host fault fixture exercises hard and torn GPT writes, synchronization and
readback failures, and successful-prefix kernel rollback. These deterministic
checks do not establish real UFS power-loss behavior. The expanded VM fixture
also exercises seven child formats, actual kernel partition ioctls and the
production dracut shutdown hook. Its device, power and final reboot substitutions
exist only in test executables; installed helpers have no generic-disk override.
Current validation results and physical limits are recorded separately below.

Observed on 25 October 2026: Fedora x86_64 host checks and disposable QEMU guests
with kernel 7.4.0-0.rc0.261023g734bef548a96.6.fc46.x86_64, a 256 GiB sparse SCSI
disk and 4 KiB logical sectors. The corrected paths passed targeted guest runs:

| Scenario | Observed result |
| --- | --- |
| Ext4 | Offline shrink, live growth, split and merge passed. |
| F2FS | Offline growth from a 100 GiB filesystem to the split size and then full userdata passed; preservation shrink remains blocked. |
| Btrfs | Live shrink/growth, split and merge passed. |
| XFS | An initially smaller filesystem fitted the split partition; live growth and merge passed with kernel geometry verification. |
| Kernel update failures | Actual BLKPG create/merge refusal restored only completed operations; old GPT and the sentinel remained intact. |
| Shutdown | Production dracut hook ran after oldroot unmount, remounted a separate journal and completed singleboot. The bootloader request was intercepted by the test. |
| Altered consent | Rehashed external consent was rejected before target writes. |

Each filesystem scenario created all seven child formats, checked before/after
GPT archives, preserved a sentinel file and UUID, and retained the OEM payload
marker. The 38 host metadata/undo fault cases and complete native host regression
suite passed. These results are not full-filesystem stress, real power-loss or
physical Uke acceptance. Tablet SELinux shutdown handoff, watchdog/power behavior,
native signed delivery and actual fastboot entry remain untested here.
