# uke-orangefox-packaging

DNF-managed OrangeFox image delivery, manifests and release tracking for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

This repository packages the published OrangeFox Uke alpha as checksum-verified
image data for DNF. Local RPM/SRPM production, extracted ramdisk/Python/privacy
audits and AArch64 DNF install, upgrade and removal tests passed. Physical boot
and rollback remain untested. See the [validation report](reports/RECOVERY-RAWHIDE-2026-10-04.json).

## Package scope

- `uke-orangefox-recovery`

Recovery source lives in [OrangeFox Uke](https://github.com/MCC45TR/orangefox_device_xiaomi_uke).

## Required validation

- Verify exact GitHub release asset hashes and source provenance.
- Keep alpha and stable release channels distinct.
- RPM scriptlets must never flash, unlock or change boot selection.

The first packaging target is Fedora Rawhide AArch64 in
[uke-linux-test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/).
DEB and Alpine APK targets require their own native rules later.
Unimplemented targets fail explicitly rather than producing placeholder RPMs.

Use `make validate` to check this repository's manifest contract. Native device
applications use C++; upstream C/assembly interfaces retain their original
languages. Host automation uses Bash or Make. Python does not enter tablet
payloads. Source archives belong in `referances/`, development in `src/`.
Public records exclude personal paths, unit identifiers and calibration data.

See [the roadmap](docs/ROADMAP.md), [source policy](docs/SOURCE-POLICY.md) and
[manifest](manifests/component.json). New material findings are recorded in
`docs/lessons/`; source, package, emulation and physical evidence stay distinct.

Use `make srpm` for reproducible delivery from the locked release assets,
`make audit` for host inspection of both image ramdisks and the ZIP, and
`make track-stable` to fetch eligible stable metadata. Official host dependencies
are Bash, Make, curl, jq, RPM build tools, GNU coreutils/tar, gzip, lz4, cpio,
binutils, ripgrep and unzip. COPR installs these only in its isolated build
environment. See [DNF delivery](docs/DELIVERY.md), [source offer](docs/SOURCE-OFFER.md)
and [automation](docs/AUTOMATION.md). The initial image is an OS3-profile alpha;
later source changes are not implicitly present in that download.
