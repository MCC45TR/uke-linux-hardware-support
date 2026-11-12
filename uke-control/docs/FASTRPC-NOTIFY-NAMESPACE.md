# FastRPC service notification and RAM preflight

Software qualification on Uke, 12 November 2026, kernel 7.3.3-24-unstable,
original Fedora systemd 262.2. The fixture uses the installed distribution
flock, systemd-notify, sleep, loader and libraries. It builds no native code
and does not open a DSP or calibration source.

The final case verifies Type=notify with NotifyAccess=main, shared flock with
no fork, a minimal RootDirectory, empty proc/sys, AF_UNIX only, empty
capabilities and MemoryDenyWriteExecute. An explicit read-only bind exposes
only the host notification socket. The main process publishes readiness and
executes a two-second wait with the same PID. Both service result and exit
status succeed. The transient unit and tree are removed.

Generic RootDirectory documentation describes automatic notification socket
mounting. The actual systemd262 execute.c function
exec_get_private_notify_socket_path returns no private path when effective
MountAPIVFS is disabled. The target fixture observes the absent socket and
ENOENT. Keep the empty proc/sys boundary and bind /run/systemd/notify explicitly.
The Linux API does not need a systemd fork.

A separate bounded tmpfs mount verifies size=16M, mode=0700 and noswap. Only a
public vector is written; it is removed by unmount. The kernel documents that
noswap disables swapping for that filesystem. This provides a measured option
for a later private trial copy without persistent calibration storage.

Initial uninstrumented cases failed before readiness. Journal diagnostics
identified the missing socket; intermediate direct-command variations and an
all-access trace were not accepted as the production contract. The final case
uses the intended flock/no-fork execution shape with main-only notification.
Its result does not establish live original FastRPC provider parity. Required
next gates include bounded automatic rollback, private registry isolation,
device access, real root/sensor readiness, SELinux and crash/suspend behavior.

Primary references:

- [systemd262 execute.c](https://github.com/systemd/systemd/blob/v262/src/core/execute.c),
  exec_get_private_notify_socket_path.
- [systemd262 namespace.c](https://github.com/systemd/systemd/blob/v262/src/core/namespace.c),
  notification socket bind mount.
- [systemd-notify options](https://github.com/systemd/systemd/blob/v262/man/systemd-notify.xml),
  exec preserves the process ID.
- [Linux tmpfs](https://docs.kernel.org/filesystems/tmpfs.html), noswap and size.
