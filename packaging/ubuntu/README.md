# Ubuntu source packaging migration

Status: 1 November 2026. Work in progress; no package is ready for publication.
Fedora RPM specs and common C++ sources are unchanged. Debian overlays select
those same tracked component sources; this is not an RPM-to-DEB conversion or a
separate Ubuntu application fork. The packaging branch can merge back without
changing Fedora recipes. Original Ubuntu KDE packages remain distribution-owned.

## Target and source preparation

Target: Ubuntu 27.04 **tenacious**, ARM64, unstable PPA only. Launchpad reports
Tenacious in active development and its official ARM64 architecture with a
virtualized builder and chroot. See the [series API](https://api.launchpad.net/1.0/ubuntu/tenacious),
[architecture API](https://api.launchpad.net/1.0/ubuntu/tenacious/arm64), and
[PPA series list](https://launchpad.net/ubuntu/+ppas). Development availability
is not stable qualification. Stable must remain empty.

Run from this repository using Bash, Git, tar, xz and dpkg-dev:

```sh
bash packaging/ubuntu/tests/source.sh
bash packaging/ubuntu/prepare-source.sh uke-battery-observer /tmp/battery-source-new
bash packaging/ubuntu/prepare-source.sh uke-front-color /tmp/color-source-new
```

The output directory must not exist. The script reads shared source commit
`cbecc441059ebffa97497e736edd2e0b0b0546a5` and writes a `3.0 (quilt)` source
package: original source tarball, Debian tarball and unsigned `.dsc`.
It never runs upstream build commands, signs, uploads or installs anything.
`SOURCE-ORIGIN` identifies the exact component and commit. A source revision
change requires deliberate pin review and an appropriate package version bump.
No external source fetch occurs during preparation or build.

## Implemented recipes

- `uke-battery-observer` 1.2.0: read-only collector only, shared source and
  systemd/D-Bus policy, dedicated sysusers identity created during configure,
  Debian shlibs dependency generation. Charge writers and offline charging are
  excluded and need separate coordinated review. D-Bus activation remains
  available; install does not enable or start the systemd unit.
- `uke-front-color` 0.1.0: native raw snapshot helper and unchanged Uke firmware
  condition and SSC lifecycle binding. Installation never enables or starts it.
  `uke-ssc-fileservice` must be ported before this package becomes installable.

These source versions come from the shared component RPM recipes. Their former
standalone COPR entries are superseded; Debian splitting does not revive them on
Fedora. Integration-package payload coverage remains incomplete.

## Coverage and migration order

`coverage.json` covers all **43** entries returned by the public COPR source
listing on this date. `source_revision` is the revision actually present in the
COPR recipe, not a claim that main equals that recipe. Null means no Git commit
was extracted (for example the archived speaker firmware SRPM).
`local-specs.tsv` additionally inventories all **30** literal local RPM spec
paths at the common-source baseline. Generated integration specs and archives
are represented by the COPR listing, not by the literal-spec count. Local specs
without an implemented Debian overlay have not yet been ported.

Current classification: 2 source-tested; 21 blocked; 9 superseded historical
entries; 6 candidates for Ubuntu originals; 4 owned by the separate desktop
migration; 1 unnecessary Fedora runtime alias. **Zero** ARM64 binary builds,
installation tests, VM tests or physical tests were performed here.

The Fedora libstdc++ alias is unnecessary: use Ubuntu `libstdc++6` with
`dpkg-shlibdeps`. AccountsService, libaccounts-glib, ddcutil, libwacom,
AT-SPI and GStreamer are candidates for distribution originals, not proven
removable forks: compare patches and Tenacious versions first. Do not remove
Mesa, PipeWire, UPower, libssc or SensorProxy patches without that comparison.
Firmware and OEM resources need provenance and redistribution review.

1. Port SSC service/dependency integration and finish the sensor/power payloads.
2. Replace `uke-control`'s libdnf5/RPM transaction and channel code with a native
   APT/dpkg implementation behind a distro boundary; retain common device APIs.
   Review Polkit identities/actions, system users, udev permissions and AppArmor
   policy explicitly; dropping SELinux policy alone does not preserve confinement.
3. Port kernel and boot integration as one cohort before any boot-affecting
   package can be admitted. Keep immutable kernel release, DTB, modules and
   initramfs identities, hardware-compatible DT checks, existing recovery and
   previous known-good boot selection. Debian dependencies must bind the exact
   cohort; never translate RPM virtual capability strings mechanically.
4. Implement initramfs-tools hooks only after auditing the existing dracut
   selection, firmware allowlist and mount/rollback behavior. No automatic
   partition, firmware, boot-entry or initramfs writes are supplied by these
   initial recipes. No Ubuntu images are created during package migration.
5. Build in an approved clean Tenacious ARM64 worker, inspect all binary payloads
   and transitive dependencies, run install/upgrade/remove and service-policy
   tests, then perform separately authorized hardware qualification.

## Publication blockers

Launchpad account identity is `mcs45tr`; GitHub identity `MCC45TR` must not be
substituted. Parent owns PPA creation/terms acceptance and key enrollment.
Neither acceptance nor source-signing authorization is established here. No key
was generated, registered or inspected; no credential or upload configuration
was created. Signed `.dsc` and source `.changes`, verified signer enrollment,
PPA write authorization, ARM64 build enablement and dependency availability
must all be confirmed before upload. Source assembly alone does not produce a
complete signed Launchpad submission. No stable promotion is permitted until
cohort, install/rollback and device qualification evidence exists.

Validation: Debian 13 x86_64, dpkg-source 1.22.22. Both source packages assembled
and extracted; source checksums, changelog target, architecture, no-start policy,
maintainer-script shell syntax and invalid/reused-output rejection passed.
No upstream code was executed. Native ARM64 builds and runtime tests require
separate execution approval under the current task constraint.
