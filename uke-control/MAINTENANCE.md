# Maintenance inventory

30 October 2026; TEST implementation, host fixtures only.

`uke-control --json storage status [/dev/DEVICE]` reads the kernel block class,
mountinfo and swap inventory without invoking a shell or writing a device.
Selecting `/dev/sda` includes its actual sysfs children. Selecting `/dev/sda1`
selects only that partition. A whole disk is never reported as a root filesystem.
The KDE widget shows this read-only inventory and leaves failed reads unknown.

Device nodes must be block devices matching the kernel major/minor identity;
symlink aliases and traversal are rejected. A disappearing or changing sysfs
identity aborts the snapshot. Identity validation is an observation, **not** a
transaction lock or protection against later hotplug/reuse. Every future mutation
must reacquire and pin the target and revalidate its identity and dependencies.
Mount paths retain mountinfo escaping and are labeled accordingly. Filesystem
names from mounted entries do not establish Fedora/Arch root identity.

`uke-control chroot plan [/dev/DEVICE]` exposes the same prerequisite inventory.
It does not mount a filesystem or execute a target program. Every record has
`chroot_ready: false`; `chroot_execution_supported` and `mutation_supported` are
false. Missing device access, unknown mounts, active swap, holders and read-only
state must not be presented as readiness. Encrypted devices, LVM, RAID and Btrfs
subvolume selection require additional discovery; this increment does not unlock
or infer their contents. Existing boot-layout/resize workflows are unchanged.

## Required before enabling chroot execution

- Identify Fedora/Arch from bounded, no-follow reads of the actual root
  filesystem, never from labels, partition names or whole-disk paths.
- Require administrator authorization and explicit disclosure: chroot shares the
  host kernel, is not a security boundary, and can run target-controlled root code.
- Inspect in a private mount namespace with propagation disabled and
  filesystem-specific read-only/no-recovery options. Reject unsupported or
  ambiguous layouts instead of replaying a journal during inspection.
- Pin block identity, revalidate after authorization, reject live/root/boot and
  busy dependencies, serialize operations and reject stale/repeated requests.
- Bind only the required host interfaces within that namespace. Do not expose
  host mounts, credentials or arbitrary helper shell arguments by default.
- Own the child process lifecycle, handle signals and failures, and report any
  busy mount/cleanup failure. Test interrupted setup and teardown on disposable
  images before any tablet execution.

Partition mutation remains a separate authenticated transaction with an exact
plan and explicit destructive confirmation. This inventory grants no permission.

## Hardware acceptance checklist

Use validated unstable COPR artifacts and approved `dnf update` deployment only.

- Confirm enforcing policy and confined broker account; active local flashlight
  calls need no sudo prompt. Remote, inactive and root-origin calls must fail.
- Verify 1/99/100 brightness mapping and readback, off, service stop, suspend,
  provider removal and broker crash. Measure the 60-second timeout under a bus
  request flood; establish physical off independently from API readback.
- Compare storage identity/mount inventory with kernel and util-linux evidence;
  refresh after removable-device replacement. Unknown access stays unknown.
- Verify the Plasma page loads, handles an absent backend, repeats refresh safely
  and never enables chroot/mutation from this planning-only response.
- Do not test destructive storage behavior on tablet disks. Chroot execution,
  filesystem detection, cleanup and destructive planning remain unimplemented.
