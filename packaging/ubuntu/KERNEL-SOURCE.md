# Shared kernel source and Ubuntu candidate boundary

Recorded 1 November 2026. The COPR producer is pinned to kernel repository
`8983163763f708ec940b349ebb81ab43cc3725c3`, release 7.3.1-115. It composes
the retained release-110 source RPM (build 11239296, SHA-256
`84a612c12c4b99ce29077273cdca6dce704fd94c364f4c7c50bcdeb9313af69c`)
with nine explicit source overlays. Git checkout alone is not that composition.

`prepare-kernel.sh` extracts source members only, verifies the Linux source hash,
retained profile manifest and every unchanged parent file, applies those exact
nine overlays by copying source files, and verifies the boot recipe's complete
source-asset manifest. It does not execute the COPR worker, native preparer,
compiler tests or boot producer. Source tests independently verify Linux's
detached signature against the retained public key and expected signer.

The `uke-linux-kernel` recipe defines three **candidate-only** ARM64 packages:
core Image/configuration, DTB and modules. All payloads live beneath
`/usr/lib/uke-kernel-candidates/7.3.1-115-uke-ubuntu1/`; exact Debian dependency
versions bind the packages. The distinct kernel release prevents confusion with
Fedora modules. No `/boot`, live `/usr/lib/modules`, service, maintainer hook,
boot selection, firmware blob or boot-capability provider is installed.

The future approved native build retains the shared preparer's tests and
configuration. It makes the first kernel Image pass with an empty embedded
initramfs while retaining `CONFIG_INITRAMFS_FORCE=y`. **That Image is not
bootable.** `debian/build-kernel boot` exits 78 before creating output. It must
not be used as an intermediate workaround for deployment.

Shared configuration enforces module signatures. A future kernel build normally
generates a disposable module-signing key, retains its public certificate and
removes the private build key after module installation. This is separate from
the user's Launchpad OpenPGP upload key. No kernel build or key generation has
been run here; the pending build approval must cover this standard build effect.

`inspect-kernel-candidate.sh` reads actual extracted content: complete digest
inventories, ARM64 Image header/release, Uke/SM7675 DT compatibility, required
configuration, module ELF architecture, vermagic and signature metadata. It
rejects links, extra unlisted files, live boot/module locations and an empty
module tree. It does not cryptographically verify module signatures or claim
full DT binding/driver qualification. Its synthetic refusal tests cover corrupt
Image bytes, foreign DT, missing modules, symlinks and traversal. No real
candidate module or kernel has yet passed this inspector.

## Remaining boot integration

The retained `make-initramfs-base.sh` builds a systemd/dracut base and records
Fedora RPM ownership/licenses. Ubuntu needs an explicitly reviewed package
provenance backend and equivalent root-selector, filesystem, timeout, offline
charge and recovery behavior. Do not remove those checks to make a build pass.
The full Fedora boot-payload test suite is not replaced by candidate packaging.
Embedded initramfs generation, complete DT/driver admission, module trust,
Android boot-v4 coherence, updater transactions and physical rollback remain
unfinished. `uke-kernel-boot` is explicitly blocked, and no recipe provides
`uke-boot-cohort-v1`.
