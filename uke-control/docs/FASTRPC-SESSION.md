# Original Fedora FastRPC session integration

Source candidate, 12 November 2026. The existing Control package owns a small
Linux session launcher and the registered-listener notification adapter.
The unmodified Fedora FastRPC library owns all RPC protocol and file services.
No new runtime RPM or Fedora rebuild is required.

The launcher accepts only the ADSP root or sensors role, requires the scoped
notify environment and notification adapter before loading the original
versioned listener, then invokes its entry once. Any return ends the process
with EX_TEMPFAIL (75). systemd can release dependencies, apply restart delay
and establish readiness in a fresh process. This avoids the distribution
daemon frontend's same-process retry loop without changing its library.
Missing scope, malformed notify socket or absent integration is refused before
a protection-domain attach. Configuration failures must not restart forever.

Only the original fastrpc library package is needed for this path. Installing
fastrpc-services would also introduce its 59/60 device/autostart rules; those
rules must not acquire Uke CDSP/audio lifecycle ownership accidentally.
Existing users of those services still need an explicit ownership transition.
The cache and root/sensor unit dependencies, namespace, original SDK library
lookup, SELinux and crash/suspend behavior need runtime qualification before
the old SSC provider can be removed. Healthy idle operation must block on the
original listener's event descriptor.

The native fixture executes this launcher with the original Fedora listener and
stubs. Only remote RPC and device-name endpoints are mocked. A mandatory
seccomp filter denies every ioctl before exec. Eight independent process cases
check role, readiness, failure, fallback, one-session exit and refusal paths.
Synthetic public files stand in for device names. It does not access a DSP.

A separate compile-time fixture compares all 13 Linux ioctl commands, 11
structure sizes/alignments and all their field offsets between the original
Qualcomm v1.0.8 header and Linux d50d994b, the cumulative kernel24 base.
The capability's first word is ignored by this kernel; spelling changes in
version/request fields do not change layout. Equality establishes source ABI
layout, not firmware semantics, DMA correctness or a successful live invocation.
The actual cumulative patches do not change this UAPI header.

References: Qualcomm FastRPC v1.0.8 d247519650fe5cb16de6c78edaa95bcc4be25073,
inc/fastrpc_ioctl.h, src/adsp_default_listener.c and src/dsprpcd.c
(BSD-3-Clause); Linux d50d994bffa3c7c6db2d8bc4c5f0c55e3f46b0cd,
include/uapi/misc/fastrpc.h (GPL-2.0 WITH Linux-syscall-note).
Reference inputs are downloaded unchanged by the source worker, pinned by
SHA256 and used only by the fixture. No OEM protocol implementation is copied.

Return path: remove the launcher/notification layer when the original Fedora
provider supplies the required one-session systemd readiness and lifecycle
contract. Preserve supported upgrades, private cache data and rollback first.
