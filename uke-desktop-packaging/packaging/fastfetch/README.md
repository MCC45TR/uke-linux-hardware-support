# Fastfetch SMBIOS fallback correction — 18 October 2026

Environment: POCO Pad X1, Fedora Rawhide AArch64, Linux7.2.11-1 and original
Fedora fastfetch2.68.1-1.fc46. A root `--config all` capture terminates with
SIGBUS in memcpy of a64KiB `/dev/mem` mapping at physical0xf0000. The fixed
legacy PC BIOS range is not an ARM device-tree firmware-discovery interface.
Transiently denying this process access to `/dev/mem` makes the full capture
complete; it does not provide a permanent application correction.

The patch returns unavailable before that fallback on non-x86 architectures.
Normal firmware-table discovery and legacy x86 behavior remain intact. It does
not change device permissions, add SMBIOS tables or fabricate missing hardware
data. Actual-function tests use synthetic I/O; AArch64/x86 compilation and
physical root/desktop-user captures are separate gates.

The source and Fedora spec are pinned in `source-lock.json`. This overlay keeps
the installed distribution version and changes its numeric release. The newer
upstream2.70.0 source17bb3fd and development bbb9254 still contain the same
unrestricted fallback at `src/common/impl/smbios.c`; no existing fix was found
in those exact compared files. The base spec is Fedora rpms/fastfetch commit
cd71d38. Upstream MIT licensing and installed license files are retained.

Upstream CMake can build without Python: its original unminified help and basic
man-page fallbacks are used, and optional embedded PCI/AMD databases stay off.
Python discovery is explicitly disabled and no Python target payload is allowed.
Build with `make srpm` in this directory. COPR11169224 passes the actual fallback
fixture and all six upstream tests. Four RPM signatures, exact source inputs,
native AArch64 hardening and absence of Python payload/dependencies pass audit.
DNF upgrades only Fastfetch to2.68.1-2 on Linux7.2.11-2; RPM verification passes.
The ordinary root all-module invocation completes at exit0 without the transient
`/dev/mem` restriction. Missing cache geometry and unavailable hardware remain
explicit errors; the correction does not manufacture those capabilities.
