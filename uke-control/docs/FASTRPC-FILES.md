# Original Fedora FastRPC file API qualification

Native candidate100,12 November2026. No provider transition or new runtime RPM.

The fixture loads the original Fedora1.0.8 libadsprpc object and calls its
exported apps_std functions directly. Neither this library nor its protocol
implementation is rebuilt. The exact RPM header and library ELF are pinned.
All ioctl syscalls are denied by a required per-process seccomp filter before
the library is loaded;the process must be unprivileged and have no ADSP node.

Public temporary data models config aliases and two writable registry paths.
The checks cover read/EOF/seek,write/append/flush,rename/remove,mkdir,and
read-only/missing/null-filename/invalid-handle refusals. They verify unchanged
config content after the registry operations. No remote RPC,real firmware,
factory persist or private calibration is used.

Fedora does not install apps_std.h. The source-only test therefore includes
that unchanged header and its BSD-3-Clause license from the exact Qualcomm
v1.0.8 archive. The installed public Fedora headers supply its dependencies.
This is a test dependency,not a project runtime API or a repackaged library.
The test also retains every native99 listener/readiness case and source input.

The ordinary fixture directory is not the target RootDirectory. This result
must be kept distinct from the separately qualified native namespace fixture,
real DSP requests and lifecycle/DNF acceptance. No file-descriptor/DMA API,
reverse-request decoder or live sensor calibration acceptance is claimed.
Those paths require their own evidence if used by Uke.

Source: [Qualcomm FastRPC v1.0.8](https://github.com/qualcomm/fastrpc/tree/d247519650fe5cb16de6c78edaa95bcc4be25073),
BSD-3-Clause;inc/apps_std.h and src/apps_std_imp.c. Existing integration and
the test fixture are MIT. Native compilation and RPM generation run in COPR.
