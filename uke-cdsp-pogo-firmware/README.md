# Device-local Uke CDSP and pogo firmware

4 November 2026: candidate native C++ provisioning helper. It reads only fixed
stock files from modem_a and the hash-admitted MCU image in stock ODM/vendor.
It never uploads OEM firmware to Git/COPR or writes a source partition.

Production requires exact Uke/SM7675 identity, root-owned descriptors, both stock
sources below the observed Uke UFS controller and a non-running installed kernel
release. Modem and super occupy different UFS LUNs. Super must share the Fedora
root disk; the modem LUN must belong to the same exact bound UFS controller.
CDSP MDT identities are pinned to the retained Global304 ELF32 headers;
split sizes follow that authenticated metadata. The kernel PAS loader still
performs secure authentication. The MCU image has an exact whole-file hash.

Firmware families publish atomically to the versioned firmware directory.
Existing differing files or symlinks are refused. No DSP or MCU is started,
no trusted app is loaded and no calibration is accessed. The helper does not
provide a DSP program/runtime or keyboard authentication token provider.

Version 0.2 adds --preflight-auth and --import-auth KERNEL_RELEASE. They
reconstruct the nine stock devauth parts in bounded memory, reject inconsistent
overlaps and admit only the retained 147,544-byte Uke image by its complete
SHA-256. They do not load the trusted app. Candidate publication now requires
a non-running installed -unstable kernel ABI.

The read-only LP/loop/debugfs path reuses the original Uke boot updater header
at a96607f. FAT parsing and bounded publication are original MIT C++ code.
Synthetic native/sanitizer tests run on COPR. Tablet provisioning and physical
operation remain separate acceptance gates.
