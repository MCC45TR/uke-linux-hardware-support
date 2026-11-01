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

## Workspace branch validation

Root Ubuntu CI 41299730079 failed at configured branch validation with exit 128.
A fresh shallow hardware-support clone plus fetch of the pinned packaging SHA
reproduced the missing `origin/ubuntu` reference. Fetching only the configured
branch ref repairs validation without moving the pinned checkout. Root commit
33e237d retains all existing tests; run 41301170081 passed the workspace job.
This is branch-awareness, not a package or hardware qualification result.

## Release source reconciliation: battery protection inventory

Recorded 1 November 2026 on Debian 13 x86_64. Lesson ID:
`ubuntu-source-release-identity-20261101`. The current Control family reuses
the released battery 1.4.1 source RPM, whereas the common Git component is
older. Its `control-profile.cpp` requires an additional
`integrated_protection_inventory` digest; readiness metadata also demands
inventory review and leaves physical acceptance false. Pin both the source
RPM and its nested source archive and reuse that source verbatim in Debian.
Never advance a version label without reconciling these safety differences.
All 16 source assembly/extraction checks pass; no upstream code was compiled
or executed. Battery ARM64 build, runtime and physical qualification remain
pending. Front-color's 1.0.2 wrapper contains 0.1.0 source with five source
files identical to the common baseline; wrapper and application versions
must be reported separately.

## SSC release, recovery delivery and APT policy

Recorded 1 November 2026, Debian 13 x86_64. Lesson IDs:
`ubuntu-ssc-release26`, `ubuntu-recovery-immutable-zip`, `ubuntu-apt-policy`.
SSC release 26 adds verified OEM-reference selection with a packaged fallback
in its cache source. The six FastRPC patches and five systemd/udev files match
the common source, but the cache and its test must come from the pinned release
RPM. Member hashes are recorded in `release-overrides.tsv`. No cache program
or upstream test was executed.

Recovery's existing Fedora package delivers Android v003 artifacts as data.
The Debian equivalent preserves this scope and has no maintainer hooks. The
first payload check failed because `dh_strip_nondeterminism` rewrote its ZIP.
An explicit override preserves the published bytes; extracted payload checksum
tests must cover all release files, not only the IMG. This is packaging work,
not recovery recompilation, boot validation or a new Ubuntu image.

New project-owned APT policy fixture tests pass using official g++ without
running any package command. Managed source parsing, interrupted dpkg state,
holds and boot/private exclusions are implemented. They deliberately provide
no transaction authorization: authenticated metadata, solver, operation journal
and boot finalization remain implementation work, documented in APT-BACKEND.md.

## Kernel composition, attributed preset and dependency closure

Recorded 1 November 2026, Debian 13 x86_64. Lesson IDs:
`ubuntu-kernel-retained-source`, `ubuntu-firmware-file-attribution`,
`ubuntu-isolated-build-closure`.

The current kernel producer at 8983163763f708ec940b349ebb81ab43cc3725c3 composes
release 115 from source RPM build 11239296 plus nine exact overlays. A Git-only
archive would lose retained release-110 sources. The Debian source preparation
now verifies the parent RPM hash, Linux archive hash, retained-file inventory,
overlay identities and boot source assets without running the producer. Linux
signature verification initially stopped because this host lacked gpgv; the
official Debian package was checksum-verified and extracted locally. All 20
source routes then passed assembly/extraction checks, including the signature.
No native kernel preparer or compiled source program ran.

Kernel packaging is deliberately candidate-only. The first-pass Image retains
INITRAMFS_FORCE but has no embedded userspace, so it cannot be deployed. Three
packages use an isolated candidate path and distinct Ubuntu kernel release;
boot production explicitly fails. Actual payload inspection checks Image/DT
content, complete inventories and module ELF/vermagic/signature metadata, with
synthetic refusal fixtures only. Module cryptographic trust and real payload
qualification remain untested. Future native builds retain the shared enforced
module-signing configuration and its disposable key-generation behavior; no
key was generated during this work.

The complete firmware review is incomplete for 210 of 217 files. The separate
4,828-byte FS19xx preset has an exact original ODM notice mapping to Apache-2.0.
Source and data-package tests verify preset, compressed notice, content ID,
license identity and conffile preservation. No unrelated firmware or private
calibration is included. Six data-only ARM64 package builds pass; this is not
hardware playback or protection qualification. The optional OEM helper remains
unported because its unconditional SELinux mount context needs shared-source
Ubuntu portability without weakening read-only/ownership/extent guards.

Official APT 3.0.3 with isolated configuration and empty status resolves 19 of
20 Build-Depends expressions against the saved Tenacious ARM64 indexes.
SensorProxy requires the unpublished patched libssc-dev version and correctly
fails. An initial attempt combined --no-download with --simulate satisfy and
failed on uncached archives before meaningful resolution; the corrected path
uses explicit simulation in both configuration and arguments. It downloaded
and installed nothing, and did not run hooks. This proves metadata solvability,
not archive authentication, native compilation or runtime closure. Retain
dependency-first libssc publication rather than weakening SensorProxy's gate.

### Native server build admission, 1 November 2026

The prepared source set was approved for clean server ARM64 compilation,
including upstream helpers and disposable kernel module-signing keys. The
private upload key remains on the tablet. GitHub's native ubuntu-24.04-arm
runner is suitable; its host release is distinct from the build root.
The official ubuntu:devel ARM64 manifest at digest 73b76549166d2d32444ccbd2b79b34381a1b3e25fd150cdd7fb12383fe7ebdc2
still reports Stonking 26.10, and ubuntu:tenacious is absent. Bootstrap
Tenacious from the signed ports archive instead and assert version plus
architecture inside the disposable container. Retain logs and package outputs,
never entire build trees that may contain transient module private keys.
Compilation and hardware qualification remain separate evidence.

### Native build and first recipe results, 1 November 2026

Run 41308345325 compiled the shared sources in clean Tenacious ARM64 server
roots, including patched libssc and SensorProxy in dependency order. The
first eight downloaded route artifacts produced ten DEBs; their full artifact
checksums and direct payload audits passed, including ELF64 AArch64 identity
and absence of Python payloads or direct Python runtime dependencies.
Image tools later passed compilation without composing a system image.
Recovery delivery passed packaging of the existing Android artifacts only.
These findings do not qualify hardware or transitive runtime closure.

The first UPower build compiled successfully, then dh_missing rejected four
unassigned API/documentation files. Retain the D-Bus interface XML in the
development package and move the configuration-directory README into daemon
documentation; do not suppress dh_missing. The corrected build is retried.

Firmware validation run 41308699685 passed native DEB build, payload checks,
install, reinstall preserving a locally changed conffile, remove preserving
that file, purge and git-build-recipe 0.3.7build1 source construction.
dpkg-source creates .pc bookkeeping even without patches; exclude that
generated state from the recipe export. An upstream Git tag lets the official
recipe tool construct its original archive without changing quilt format.
The published launchpad/uke-speaker-firmware branch and upstream/3.0.304.0 tag
preserve shared source bytes. Launchpad import and recipe creation remain
pending in the parent UI session, not blocked on a server private GPG key.

UPower retry 41309136146 passed after the install-list correction, including
all four binary packages and direct payload audits. The initial matrix passed
18 of 20 routes; this retry brings qualified server construction to 19 of 20.
Six routes are data-only and thirteen compile native payloads. Kernel attempt
124853346238 stopped when the unchanged shared preparer required
senemos-applied-patches.txt, which the Debian driver had omitted. Record only
successfully applied patches, compare the full ledger against the verified
series, and retain all shared tests. Retry 41309327976 remains active; no
kernel binary or boot result is inferred from its preparer fixture tests.

### Paused checkpoint after terminal kernel retry

Kernel run 41309327976 terminated on 1 November with a real compiler failure
in sound/soc/qcom/uke-audio-card.c:411: the shared assertion limits
sizeof("uke" UTS_RELEASE) to 16, while the Debian kernel release string
7.3.1-115-uke-ubuntu1 is too long. Patch-ledger preparation passed. The next
implementation action is a shorter coordinated Debian release identity with
matching candidate manifests and inspectors, retaining the shared assertion;
no fix or new dispatch was made after the user's Fedora Rawhide DE priority
change. Nineteen non-kernel routes remain native-build-passed. All nineteen
recipe source branches are published with exact-original pristine-tar metadata.
The two-package colliding-version regression passes. Runtime metadata simulation
of all 27 actual DEBs resolves 16 closures and refuses 11 missing real ABI
providers; two resolved development closures include host Python and remain
server-only. Launchpad recipebuild 4108742 is currently building source; it is
not evidence of an ARM64 binary publication. New Ubuntu work is paused and
existing remote builds are monitored read-only.

### Resumed experimental kernel and minimal userspace preparation

The new user request authorizes preparing an Ubuntu test image, with no tablet
writes. Main unstable still publishes 115. Release1006 was found in the separate
uke-linux-test:custom:power-20261031 channel, successful COPR build 11240463,
source commit fcbdae4b757732f0f0e3162c610f19abae290832. Preserve its exact emitted
retained1003-plus-LMH composition rather than treating the current main Git
checkout or a numerically newer failed trial as equivalent. The source RPM
SHA256 is 70f151855f727c9d83c3bedc35ff52dd60cb222b2d741818472dced458bcd60c.
Its profile differs from the previously approved 115 in 28 files, including
LMH, SCMI/CPU observation and O82 VRR behavior. No new source helper executed.

The Debian recipe now stages that exact source, preserves original inventories
without recomputing away mismatches, and uses 7.3.1-1006u1. Its shared ALSA
identifier including prefix and NUL is exactly16 bytes. The recipe, candidate
inspector and fixtures agree; source checks reject oversized identities early.
Signed Linux source is represented as80MiB bounded chunks for Git service
compatibility. Their joined bytes retain the exact upstream digest and detached
signature. Source assembly, emitted inventories, full source checksums, signer
identity and candidate refusal fixtures passed. No source build or boot passed.

Boot inspection found two concrete port tasks: the shared producer accepts
numeric Fedora release arguments only, and its userspace SBOM uses RPM ownership
and licenses. Preserve root-selector, module, recovery and privacy checks while
adding reviewed Debian integration. Do not promote the empty-initramfs candidate
Image. The full cohort must retain one module key across both Image passes,
then remove its private key before export; the existing candidate-only action
deletes it after module packaging and cannot safely be promoted later.

The shared kernel selects SELinux and lacks AppArmor. Official Tenacious
dracut-core112-7 and SELinux packages3.11 exist. APT simulation of policycoreutils,
selinux-utils and selinux-policy-default resolves without Python. A disposable
native ARM64 userspace workflow installs only official runtime packages and the
exact published PPA preset, verifies the existing archive key, checks Python
exclusion and records policy/payload evidence. It emits no tablet image or boot
bundle. Policy labeling/enforcing handoff and real hardware boot remain separate
unimplemented checks, not signing or credential blockers.

### Host adapter and userspace qualification checkpoint

Native ARM64 userspace run41319778138 passed authenticated official Ubuntu/PPA
installation, preset identity and Python exclusion. First trials found dormant
Apport hooks and libstdc++ GDB helpers, including files initially extracted by
debootstrap before dpkg filters. The minimal target policy now records and
omits only those identified optional diagnostic files, preserving distro ELF
libraries and retaining a terminal audit for other scripts. No root image was
exported, and no MAC/enforcement configuration was selected.

The new host overlay stages against exact boot lock
dc2f9a7f0ded526e191147402403b9f66998004831cdec85d7756070cb1d4df4.
Original source assets are checked before copying. Three host-integration
changes add the bounded Debian suffix, dpkg copyright/source inventory and a
distro-neutral description; nostrip preserves package bytes for provenance.
The original lock stays intact and an explicit overlay receipt records changed
hashes. Kernel configuration, root guard, switch-root and userdata code compare
byte for byte with the shared input. No new1006 script/helper executed.

The dpkg backend resolves usrmerge/alternatives, Multi-Arch owners and relocated
dracut content. It compares selected bytes and records generated metadata
honestly. Real isolated-dpkg metadata fixtures pass and refuse changed owned
bytes, unknown ELF, non-executable interpreter scripts and foreign architecture.
The fixture initially omitted dpkg info/format1, making Multi-Arch list lookup
use the legacy unqualified path; the corrected fixture explicitly initializes
the database format. This was fixture setup, not a relaxed production check.
Actual native dracut/provenance execution remains pending source approval.

Ubuntu's documented default MAC is AppArmor; SELinux is an optional userspace
stack and not equivalent integration. The current shared source lacks AppArmor.
The optional SELinux package test was a compatibility probe, not authority to
force a Fedora image policy. No security setting is changed by the overlay.
The final profile requires a separately approved, validated decision before
root/boot artifacts can be emitted. Refer to Ubuntu's privilege-restriction
documentation and BOOT-ADAPTER.md for the concrete boundary.
