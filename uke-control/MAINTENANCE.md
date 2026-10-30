# Hardware and maintenance center

30 October 2026; TEST source, host fixtures and isolated kernel-namespace tests.
No tablet installation or hardware acceptance is established.

## Available commands

```sh
uke-control hub status
uke-control storage status /dev/sda
uke-control storage plan /dev/sda1 resize 34359738368
uke-control storage plan /dev/sda1 format ext4
uke-control storage plan /dev/sda1 delete
uke-control chroot manage
uke-control chroot scan /dev/sda --json
uke-control chroot enter /dev/sda1 TOKEN --trust-target-root-code
```

The KDE panel shows live kernel CPU, memory, battery and thermal observations.
Available samples, absent kernel interfaces, unknown values and read errors are
separate states. An exported interface is not physical acceptance. The maintenance
button opens the distribution's Konsole; it never embeds a root shell in Plasma.
Hardware refresh and flashlight control remain usable while the terminal is open.

## Chroot authorization and target trust

`manage` provides a terminal disk/root selection workflow. `scan` lists actual
partitions beneath a selected disk and inspects each independently. A whole disk
is never accepted as a root filesystem. A separate Polkit action binds to the
fixed native helper. Active local users must authenticate as an administrator;
inactive and remote sessions receive no implicit permission. There is no cached
`auth_admin_keep`, sudoers change, arbitrary command argument or writable-root
option. Direct root execution already requires administrator authority.

Entering additionally requires the explicit trust flag and typing `ENTER` with
the selected path. **The session executes target-controlled code as UID 0 and
shares the host kernel. Chroot is not a security boundary.** Only trusted target
systems should be inspected or entered. Mounting also parses filesystem metadata
in the kernel; read-only flags do not make a malicious filesystem safe.

Only direct, non-overlapping ext4 and F2FS partitions with Fedora/Arch `ID` and a
native 64-bit, non-setid, non-file-capability `/bin/bash` are admitted. Root metadata
is read with bounded `openat2` resolution inside the mounted root; symlink escapes,
magic links, FIFOs, oversized metadata and foreign ELF architectures are refused.
Encrypted, LVM/RAID, Btrfs subvolume/multi-device, XFS and split-root layouts need
additional discovery and are explicitly unsupported. No donor partition offsets
or labels are treated as system identity.

The manager verifies the block node against sysfs, pins its descriptor, takes an
exclusive advisory lock, checks kernel disk sequence and size, and probes UUID and
filesystem type with libblkid. The scan token includes those observations, root
metadata and the current boot identity. Enter repeats inspection and compares the
token. Mount and swap dependencies are rechecked through the original mount
namespace inventory. Mounted/live root/boot targets, overlapping extents and
block holders are refused; unrelated zram and non-overlapping sibling mounts need
not be stopped. Unknown dependencies refuse admission. A global helper lock
rejects concurrent inspections/sessions.

These checks are not an exclusive kernel fence against another administrator
writing raw blocks or racing an independent mount. The mounted filesystem and
root directory are pinned, but malicious concurrent host-root activity remains
outside this tool's trust boundary.

## Session lifecycle

Inspection uses a private mount namespace with propagation disabled and ext4
`ro,noload` or F2FS `ro,norecovery`. The private `/run` overlay leaves no host
staging directory even after a process crash. Normal inspection restores the
original mount namespace before returning or scanning another partition.

Execution keeps the root read-only and nosuid/nodev. It provides private runtime
and temporary tmpfs mounts, a read-only private process view and read-only sysfs.
Read-only proc prevents UID-0 writes to global sysctls without relying on every
individual proc handler to check capabilities. Only null,
zero, random, urandom and tty are bind-mounted from host devices; there are no host
block devices, home directories, runtime sockets or copied resolver credentials.
A new network namespace has no configured connectivity. `/bin/bash --noprofile
--norc -i` is the only executable entry point; all inherited descriptors and the
ambient environment are removed. The target receives no capabilities and cannot
gain them through exec (`no_new_privs` and an empty capability bounding set).

A trusted PID-namespace init remains outside target code. It is non-dumpable,
bound to the manager by a parent-death signal and pidfd startup check, and reaps
background jobs. On shell exit or manager interruption its namespace is terminated
and the parent reports the real shell result. Mounts are unmounted in reverse
order; busy or failed cleanup is reported, never labeled successful. Kernel I/O
that remains uninterruptible can delay teardown; a userspace manager cannot
promise recovery from every device/driver failure.

This is a usable **read-only diagnostic chroot**, not a package-repair or writable
root service. Writable sessions require a separately reviewed recovery and
filesystem transaction design. There is no silent fallback to a less protected
session when a kernel namespace, pidfd, openat2 or mount operation is unavailable.

## Partition operation planning

Plans identify the partition, current and requested geometry in 512-byte sectors,
actual logical-sector alignment, disk boundaries and sibling overlap. Format plans
name the requested filesystem. Plans list live-parent, offline filesystem support,
backup/recovery and explicit confirmation requirements. Large geometry values are
encoded as decimal strings to preserve precision in clients.

**No new destructive partition executor is provided.** Existing boot-layout and
resize workflows are unchanged. The plan's `execution_supported: false` is final;
a plan is not an authorization token. Destructive execution remains blocked until
filesystem-specific shrink/grow checks, recovery validation and interruption-safe
transactions have been implemented and tested on disposable images. The inventory
response's `chroot_ready: false` means inspection/authorization is still required;
only the separate authenticated scan can report an admitted root.

## Validation and deployment checklist

- Run `make check` and KDE `test-applet` / `test-applet-load` for source checks.
- Run `build-chroot-integration` on a host with static C++ libraries, then the
  `test-chroot-namespace`, `test-chroot-lifecycle` and `test-chroot-session
  /path/to/chroot-target-fixture` binaries in a disposable container with
  CAP_SYS_ADMIN. They use only tmpfs and project-owned native mocks; they never
  execute distribution target code or access a block device.
- Confirm compiled policy/package ownership and the exact narrow Polkit path
  before an approved unstable COPR deployment. Source tests do not activate policy.
- On approved hardware, verify actual authorization, enforcing SELinux behavior,
  filesystem admission, console operation, repeated selection, hotplug, cancellation
  and cleanup with disposable external media before considering tablet roots.
- Verify flashlight current steps, uncertain-write fail-off, timeout, suspend and
  crash physically; a kernel readback is not proof of visible/electrical off.
- Deploy only exact validated unstable COPR artifacts through approved `dnf update`.

Implementation references: [ext4 read-only journal behavior](https://docs.kernel.org/admin-guide/ext4.html),
[F2FS mount options](https://docs.kernel.org/filesystems/f2fs.html),
[PID namespace lifetime](https://man7.org/linux/man-pages/man7/pid_namespaces.7.html),
[openat2 resolution](https://man7.org/linux/man-pages/man2/openat2.2.html), and
[Linux proc mount-namespace pinning](https://github.com/torvalds/linux/blob/master/fs/proc_namespace.c).
