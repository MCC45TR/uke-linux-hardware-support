# UKE-UBUNTU-20261101: separate source packaging from device admission

Date: 1 November 2026. Environment: Debian 13 x86_64; dpkg-source 1.22.22.

Evidence: the public mcc45tr/uke-linux-unstable COPR listing contains 43 source
entries, including superseded standalone recipes. Common source commit
cbecc441059ebffa97497e736edd2e0b0b0546a5 has no existing Debian packaging.
The Control RPM explicitly requires libdnf5, RPM, dracut and SELinux integration;
translating dependency names cannot port its transaction implementation.

Resolution: add Debian overlays without changing shared C++ or Fedora specs.
Unsigned battery-observer and front-color source packages assemble/extract with
checksums and no install-time systemd activation. The latter still depends on
an unported SSC service. All other source routes retain explicit coverage states.
Tests: packaging/ubuntu/tests/source.sh passes on the stated host; no upstream
binary was built or executed, and no physical acceptance is inferred.

Uncertainty: target dependency resolution, ARM64 build/install behavior, exact
kernel/DTB/initramfs cohort admission and current firmware redistribution rights.
Next validation: approved Tenacious ARM64 builds and service lifecycle tests;
port APT/dpkg transaction integration and boot cohorts before publication.
Parent subsequently confirmed PPA terms accepted and ARM64-only PPAs created;
source-signing key enrollment and signed upload remain pending.

## Subsequent source-port implementation

Expanded to fourteen Debian source routes, including native service policy,
UCM/KMix/module data, an admission-gated core meta selection, SSC, libssc,
SensorProxy and the VA backend. Verified the official Tenacious ARM64 package
indexes: distribution PipeWire/KDE/DDC versions do not yet satisfy all shared
requirements. This is now a concrete dependency record, not an assumed blocker.
Four data-only ARM64 DEBs build with official debhelper 14.5ubuntu1 on Debian
13 x86_64; exact UCM/link, launcher, module-policy and core-profile payload
checks pass. Build-dependency resolution was intentionally not claimed.

The upstream VA archive contained its own Debian directory. The initial source
assembly caught the differing libva-v4l2 source name. Reviewed its control,
rules and copyright, retained that directory as source evidence, and corrected
overlay placement. Its six shared patches now apply through quilt. Across SSC,
libssc, SensorProxy and VA, all 21 shared patches apply on extraction. No
upstream compiler, executable, generator or native contract was run.

Added metadata-only boot cohort checks and explicit negative fixtures. The
first fixture creation exposed inherited private directory modes; fixtures now
set umask 022 before dpkg-deb. Foreign series/architecture/device, mismatched
versions, promotion, automatic boot selection, traversal and DT corruption are
rejected. These tests do not inspect real kernel/DT/initramfs contents and must
not be treated as boot admission.

Baseline-to-current COPR family reconciliation remains unfinished; the current
core selection deliberately retains the >=1.4.1 battery requirement although
the initial component baseline recipe is 1.2.0. Source-only ports are not
qualified replacements for later family releases. The missing tracked THP
source also remains an implementation/provenance gap. Signing is independent.

## Superseding touchscreen source-gap finding

The missing tracked THP directory is generated, not an unavailable donor.
Reviewed `touch-thp-source.json` and `prepare-touch-core.sh`; recovered the two
source files and Apache license at upstream commit
34046210932d654a4c0df0121ecc31c008f8148c. Input hashes and both transformed
output hashes match the shared recipe after its four static patches. This
supersedes the earlier missing-source blocker. Added a Debian touchscreen
source recipe without private calibration or donor execution. Build/runtime and
current-family reconciliation are still pending.

Expanded the power baseline's source recipe to retain separate charge-control
and offline-charge binaries, their disabled/gated activation, and the exact
UPower/kernel/boot ABI prerequisites. Added UPower 1.91.5 with all five shared
patches, original Ubuntu binary names, Polkit data and standard service lifecycle.
All sixteen source routes pass extraction checks. The five external archives
use 26 quilt patches; the THP source importer applies four additional patches.

GitHub commit 85ced4b passed both Ubuntu source packaging contracts (run
41300695468) and Hardware source contracts (run 41300695434). These results
cover the fourteen-source snapshot; subsequent sources need the next CI run.
