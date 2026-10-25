# Boot layout conversion

25 October 2026. Uke Fedora source implementation. Source, disposable VM tests,
package composition and physical tablet acceptance are separate results.

Run `sudo uke-control multiboot` or `sudo uke-control singleboot` in a terminal.
The original distribution `dialog` supplies keyboard navigation and mouse support.
`--dry-run` runs the selection and review steps and stores private metadata on the
selected external journal; it does not prepare target writes or reboot.
`singleboot status` and `multiboot status` are read-only hardware admission queries.
Use explicit `--json` for machine-readable status.

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
├─ Verify durable private journal on a separate direct ext4/XFS device
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
│  │  ├─ Mounted Btrfs/F2FS shrink or Ext4/Btrfs/XFS growth → native live operation
│  │  └─ Unsupported preservation → block; never silently erase
│  ├─ Erase userdata or existing children → two new plan-bound approvals
│  └─ Active affected swap/holders, removed root or formatted mounted root → shutdown
├─ Revalidate the sealed plan, exact device, original proof and current-boot consent
├─ Execute once with an external phase journal
│  ├─ Shrink filesystem first; prove it fits the proposed userdata boundary
│  ├─ Direct route → update kernel partition ranges before GPT commit
│  │  └─ Kernel refusal → retain old GPT, restore kernel ranges and require fresh plan
│  ├─ Commit backup GPT, then primary GPT; synchronize and read back each copy
│  ├─ Grow preserved filesystem only after its partition has grown
│  └─ Initialize selected volumes through individually bounded device-mapper views
├─ Verify both GPT copies, OEM records, filesystem geometry and preserved UUID
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
geometry. XFS shrink is unavailable. A filesystem already small enough can fit a
smaller partition without a filesystem shrink. Kernel or checker failure stops
the operation; it never chooses destructive recreation. Offline checks do not
repair corruption automatically. NTFS checking uses `ntfsfix`, which is not a
replacement for Windows `chkdsk`.

An external root-private directory on an independent ext4/XFS disk is mandatory.
It holds original GPT proof, the sealed plan, approvals, operation phases and the
completed child-ownership receipt. Keep that directory for future conversion.
Existing layouts without suitable provenance are refused, including layouts whose
names merely resemble Uke roles. Metadata backups do not contain user files.

Shutdown work uses a temporary audited dracut RAM environment, a current-boot
approval with a ten-minute preparation lease, existing kernel filesystem ioctls
and the bound nvmem bootloader restart interface. It adds no permanent startup
service or kernel workqueue. An existing shutdown environment is preserved. An
active watchdog without an admitted RAM keeper blocks this route. The shutdown
worker independently revalidates the plan after oldroot is unmounted. It never
continues a partially started job automatically. A refused reboot request
withdraws the prepared task.

A transient systemd worker and a shutdown/sleep inhibitor protect active work
from terminal closure and ordinary shutdown. Critical storage tools are waited
for without a destructive timeout. Power loss and hardware I/O faults remain
possible; maintain a file backup and recovery route. GPT restoration can only
recover metadata before formatting or growth; erased files cannot be restored
from these records.

The manager does not install an operating system, populate an ESP, recreate
Android encryption keys, or change OEM boot/firmware/slot records. Its completion
instructions identify the system-image destination and require a matching boot
image root contract. A layout containing a replaced running root must finish in
fastboot, where the operator flashes the appropriate system image.

Host-only checks: `make check-layout`. Disposable guest integration checks:
`bash tests/layout-vm.sh`. The VM fixture is excluded from installed RPMs and
has no production device-selection override. Physical Uke conversion, shutdown
handoff, fastboot entry and file preservation still require tablet acceptance.

Observed on 25 October 2026: disposable QEMU x86_64 guest with Fedora kernel
7.4.0-0.rc0.261023g734bef548a96.6.fc46.x86_64, a 256 GiB sparse SCSI disk and
4 KiB logical sectors. Ext4 offline shrink and live growth, F2FS live shrink and
offline growth, and Btrfs live shrink/growth completed. Each run created and
removed child volumes, preserved a file and filesystem UUID, retained the OEM
payload marker and verified both GPT copies. The fixture bypasses only Uke/power
admission inside its test executable; installed helpers have no such override.
These observations do not establish Uke hardware or shutdown/fastboot acceptance.
