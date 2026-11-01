# Debian boot cohort admission boundary

`validate-cohort.sh` implements artifact identity checks only; it never writes
partitions, regenerates initramfs, changes boot selection or installs packages.
The JSON schema is validated in the script. Its four roles are kernel, DTB,
modules and boot integration, all with one exact Debian version and ARM64
architecture. Boot integration must depend on each of the other three packages
with `= version`; artifact SHA256 identities are mandatory. Tenacious,
`xiaomi,uke`, explicit singleboot/multiboot layout, no automatic boot selection
and no stable admission are mandatory. No production cohort manifest exists yet.

`tests/cohort.sh` builds empty, clearly named metadata fixtures with dpkg-deb.
It tests the metadata closure and rejects foreign series, architecture or device,
a changed version, path traversal, stable promotion, automatic boot selection
and corrupted DT-package bytes. Passing this check does not validate the
contents of a kernel, a DTB, a module tree or an initramfs.

Remaining implementation is explicit: extract each real package without
executing scripts; bind its kernel release and module vermagic to the DTB and
layout; validate required Uke DT properties and platform drivers; construct an
initramfs-tools hook with the reviewed firmware/module allowlist; prove its
mount and root-selector logic on fixtures; preserve the installed selector and
previous cohort until separately admitted. Install-time kernel/boot triggers
must stage candidates rather than select or flash them. Exact rollback and
power-loss behavior need tests before emitting `uke-boot-cohort-v1`.

Virtual ABI dependencies in the initial recipes are fail-closed migration
contracts, not claims that a provider already exists. No package currently
provides the boot, touchscreen, GPU or speaker kernel ABIs. Do not add those
Provides fields until the reviewed kernel/DT/module payload is demonstrated.
