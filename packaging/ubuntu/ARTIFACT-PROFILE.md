# Reusing the verified 1006 native boot cohort

Recorded 1 November 2026. The existing clean Tenacious ARM64 producer run
41331215146 already contains a matching Image, embedded initramfs, signed
modules, compile-time DT and Android boot wrappers from source
`fcbdae4b757732f0f0e3162c610f19abae290832`. Recompilation is unnecessary to
test Debian packaging of those bytes. The original artifact ZIP is unchanged;
the recovered IKCONFIG matches its recorded configuration digest.

The explicit Debian build profile `pkg.uke-linux-kernel.artifact-trial` emits
four inert packages: `uke-kernel-trial-{core,dtbs,modules,boot}`. The boot role
depends on the other three at the exact same Debian version. Their payloads
remain under `/usr/lib/uke-kernel-trials/7.3.1-1006u1/` and documentation paths.
They install no live kernel/module tree, hook, selector or runtime ABI provider.
Default source builds still emit the three first-pass candidate packages;
this profile does not complete that source-build/boot integration.

`verified-artifacts/1006-native.json` fixes the source, producer run, original
ZIP digest and fourteen input sizes/hashes. The importer verifies those inputs,
copies only those files into a private build snapshot, verifies the copies and
runs the shared boot-trial inspectors. It cannot regenerate an Image, compile
source, create a signing key, install a package or select a boot entry. The
source producer itself refuses all actions while this profile is selected.
Missing inputs fail before source preparation. Profile fixtures use clearly
marked synthetic data, never a substitute kernel or firmware provider.

To reproduce in a disposable server source tree with the already authorized
artifact inputs available, supply all four variables explicitly:

```sh
export UKE_TRIAL_COHORT=/inputs/cohort
export UKE_TRIAL_EVIDENCE=/inputs/evidence
export UKE_TRIAL_EARLY_MODULES=/inputs/early-modules.list
export UKE_TRIAL_LAYOUT=singleboot # or the separately selected multiboot layout
dpkg-buildpackage -b -us -uc -a arm64 -Ppkg.uke-linux-kernel.artifact-trial
```

These paths contain only the reviewed existing artifact/evidence files. This
is a local binary packaging trial, not a Launchpad recipe or source upload.
No private module key or upload key is required. The exporter includes common
inspection scripts and the receipt inside the Debian source package without
maintaining a second copy of their implementation.

The actual profile passed on an official Ubuntu24 x86_64 host tool container
with the native ARM64 artifact mounted read-only and networking disabled.
Debhelper/dpkg produced four ARM64 DEBs without bypassing Build-Depends.
All 823 module signatures, 90 early-driver entries, 318 base-file hashes and
116 links passed again. Final readback compares every payload byte, mode,
type and symlink, exact dependencies and control members. It rejects ABI
Provides, hooks, unexpected files and weakened version constraints. This
cross-host packaging result is separate from the original native kernel build.

The trial's root-image, redistribution, publication and tablet-action admission
flags remain false. See [the eleven binary dependency chains](ABI-GAPS.md).
No bootable Ubuntu image, physical test, stable promotion or upload is implied.
The original artifact lacks a complete embedded-userspace license collection;
each role retains an explicit redistribution-blocked notice. That collection
is required separately before any binary publication.
