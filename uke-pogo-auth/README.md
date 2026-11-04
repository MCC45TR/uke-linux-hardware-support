# Uke pogo authentication provider candidate

4 November 2026: original native C++ implementation for the versioned kernel
auth_request/auth_token handoff. Compilation and synthetic tests run on COPR.
Exact accessory authentication and the Uke TA/challenge contract still require
device qualification.

The client admits Uke/SM7675, an -unstable ABI, the exact NanoSIC OF path and
bound driver, QTEE implementation ID 4 with object references, and only the
complete hash-admitted 147,544-byte Uke devauth image. Provision that image
with uke-cdsp-pogo-firmware --import-auth for a non-running installed candidate.
--check KERNEL_RELEASE verifies the local image without opening a TEE object.

--serve runs in the foreground; the packaged service is static and manual.
Start the provider before reconnecting the exact keyboard. It submits a token
only after successful TEE/object/TA checks and a fresh full challenge snapshot.
TA status 0xaa00000d is refused, just like other nonzero statuses. Truncated,
unchanged, replaced or oversized replies are refused. A four-second userspace
budget and the kernel's own deadline reject stale results; this does not bound
secure-world execution. Kernel acceptance of a submitted token is separate from
the keyboard's acceptance of authentication.

Object ownership releases app/loader/client/root in order. Private callback
workers belong to the original MINK client. This package does not register a
second system RPMB listener, start a filesystem listener or provide OEM firmware.
An already qualified system listener remains a separate dependency if requested
by the TA. UID, challenge and token bytes are neither logged nor published.

The shared protocol and functional object-ABI comparison is pinned to
ianchb/sheng_devauth bac03b5bcfefa1467ebad490edd910a6cef8a207. Its main program,
generated headers and precompiled libraries are not copied. Original Qualcomm
MINK/QCOMTEE and QCBOR sources are pinned in manifests/upstreams.json; their
licenses/notices ship with the package. No upstream build script or implicit
IDL compiler download is used.
