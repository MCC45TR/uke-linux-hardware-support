# Debian boot cohort admission boundary

The [kernel source recipe](KERNEL-SOURCE.md) now stages the exact experimental
test-channel release-1006 composition after the resumed test-image request.
Its approved native server trial now built the full two-pass boot cohort.
The ordinary DEB recipe continues to produce only qualification candidates.
The separate
`inspect-kernel-candidate.sh` checks actual Image/DT/module content; its refusal
fixtures pass. Full-producer artifact inspection additionally verifies the
embedded initramfs, all 823 module signatures and both wrapper readbacks.
These additions do not turn the four-package boot metadata check below into
boot approval. `uke-kernel-boot` remains explicitly unsupported.

`validate-cohort.sh` implements artifact identity checks only; it never writes
partitions, regenerates initramfs, changes boot selection or installs packages.
The JSON schema is validated in the script. Its four roles are kernel, DTB,
modules and boot integration, all with one exact Debian version and ARM64
architecture. Boot integration must depend on each of the other three packages
with `= version`; artifact SHA256 identities are mandatory. Tenacious,
`xiaomi,uke`, explicit singleboot/multiboot layout, no automatic boot selection
and no stable admission are mandatory. No production cohort manifest exists yet.

`package-boot-trial.sh` assembles four inert ARM64 trial DEBs after
`inspect-boot-trial.sh` verifies the complete producer output and provenance.
All roles share one exact version; boot depends on each other role with `=`.
Completed DEBs are re-extracted and every staged payload digest is read back.
They live only under `/usr/lib/uke-kernel-trials/RELEASE`, carry no maintainer
scripts or ABI Provides, and never populate `/boot` or the live module tree.
This is packaging of checked server artifacts, **not** a second native source
build or a Launchpad kernel qualification. Local four-role packaging passed.
Trial artifacts remain private; complete boot-userspace licensing collection
and source-build integration must be finished before redistribution.

`tests/cohort.sh` builds empty, clearly named metadata fixtures with dpkg-deb.
It tests the metadata closure and rejects foreign series, architecture or device,
a changed version, path traversal, stable promotion, automatic boot selection
and corrupted DT-package bytes. Passing this check does not validate the
contents of a kernel, a DTB, a module tree or an initramfs.

Remaining implementation is explicit: bind the qualified trial packages to
the selected root layout and runtime OEM DT; validate platform drivers; prove
root policy, security labeling and its
mount and root-selector logic on fixtures; preserve the installed selector and
previous cohort until separately admitted. Install-time kernel/boot triggers
must stage candidates rather than select or flash them. Exact rollback and
power-loss behavior need tests before emitting `uke-boot-cohort-v1`.

Virtual ABI dependencies in the initial recipes are fail-closed migration
contracts, not claims that a provider already exists. No package currently
provides the boot, touchscreen, GPU or speaker kernel ABIs. Do not add those
Provides fields until the reviewed kernel/DT/module payload is demonstrated.
