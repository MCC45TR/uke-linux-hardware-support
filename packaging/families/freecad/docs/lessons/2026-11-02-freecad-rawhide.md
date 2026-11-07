# UKE-FREECAD-20261102: isolate the requested application in its own source family

Date: 2 November 2026.
Environment: isolated Fedora 46 x86_64 host container, official Rawhide AArch64
repository metadata, FreeCAD 1.1.4 release source. No tablet was accessed.

## Evidence and consequence

Fedora's FreeCAD retirement marker cites Python 3.12 installation failure and
FESCo retirement. Package review
[2386504](https://bugzilla.redhat.com/show_bug.cgi?id=2386504) remains open and
contains the newer 1.1.x source spec. The July comments discuss retired SoQt and
SIMVoleon dependencies. Current Rawhide AArch64 metadata supplies Coin4, Pivy,
PySide6, OndselSolver and E57Format. The absence of the main package cannot be
reported as evidence of an AArch64 compiler failure.

Use Elaine Gibson's review spec as a packaging baseline, update to the current
stable FreeCAD 1.1.4 source and pin its full release hash. The release archive's
SHA-256 is
`674f2c42dc060b71c8b052c70b04a9a995dde6cb868fe568615d3ff3f2717748`,
which matches the upstream checksum file. The release tag resolves to
`4fd3bf320d9566a27e60069fc8387448aaa3a094`.

Three upstream review patches apply without fuzz. They locate external E57Format
and fix TechDraw preference initialization and parallel FileInfo tests. The
main-branch external JSON/kdtree patch does not apply to stable 1.1.4; retain the
stable headers instead of forcing unrelated changes. Declare bundled libraries
and preserve the Artistic 2.0 and MIT license texts. Workbench Python remains
upstream functionality explicitly requested for this optional application; no
project-owned Python or early-boot payload is introduced.

The review spec contains an obsolete `-b1` setup argument and omits some direct
build requirements. Use the single rootless release archive, explicit Coin4,
VTK, Orocos KDL, Qt tools and PySide tools requirements, and enable the upstream
developer tests with the offscreen Qt platform. Keep binary build networking
disabled. Source downloads occur only while generating the source RPM and are
checked against the source lock.

## Observed validation

Two source RPM builds from pinned inputs produced identical SHA-256 results.
Source preparation applies all three upstream patches cleanly. An empty
AArch64 install-root dependency transaction resolves all BuildRequires from
official Fedora Rawhide repositories: 936 packages, 871 MiB compressed. This
was a solver-only transaction; no AArch64 program was executed on the host.

CMake configuration succeeds on x86_64 with GCC 16.2.1, Python 3.15.2,
Qt/PySide 6.12.0, Pivy 0.6.10, SWIG 4.5.1 and Coin 4.0.10. Its Pivy/SWIG runtime
check passes. That host result does not establish an AArch64 binary build or
functional tablet support. The RPM's CTest suite and metadata checks are gates
for the later native build, not claimed results at this stage.

The host FreeCADBase target also compiles and links with Python 3.15. Continuing
the generated CMake build directly initially omitted RPM_ARCH, which Fedora's
linker package-note configuration requires. Restoring the normal RPM environment
resolved that host-only invocation error without changing upstream code. The
remaining application and test targets were not built on the host.

COPR build 11246334 checked out the pinned source successfully but failed in
the source-contract script inside its Fedora 44 AArch64 source chroot, before
the binary build. The same script passes in Fedora 44 x86_64 and Rawhide x86_64.
Source validation now reports its failing line and RPM identity mismatch;
the dedicated CI checks both Fedora 44 and Rawhide on x86_64 and AArch64.
Diagnostic build 11246374 identified the mismatch: Mock restored `.fc44` after
the query's `--undefine dist`, producing `1.fc44` where the base-release check
expected `1`. An explicit empty `--define 'dist %{nil}'` fixes that identity
query while preserving the real binary RPM distribution suffix. A user dist
macro fixture reproduces the failure and passes with the explicit definition.
Checksum and channel checks stay enabled.

COPR build 11246389 passed source generation and dependency installation, then
stopped before compilation because the standalone `limit_build` macro expanded
to `-j2`. Rawhide's installed macro definition returns job flags; it is not a
shell command. Append its result to `_smp_mflags`, which `cmake_build` consumes.
Expanded-spec regression checks force a constrained job limit, require the
numeric flag on the CMake build command and reject standalone job flags.

The corrected native build reached C++ compilation. Build 11246440 then failed
in `src/Gui/PythonWrapper.cpp`: PySide 6.12 moved `PyObjectWrapper` into a separate
header. Upstream [PR #33408](https://github.com/FreeCAD/FreeCAD/pull/33408), commit
`636365fd4a30e017c76ef6a44cf045e77ec5e632`, adds a conditional header include while
preserving the Quantity converter and earlier PySide support. Pin that upstream
patch and increase the RPM Release to 2; the upstream version remains 1.1.4.
All four patches apply with zero fuzz, and the actual failing translation unit
compiles against Fedora Rawhide's Qt/PySide 6.12 headers on the x86_64 host.
The next native AArch64 build and its complete test suite remain separate.

Build 11246564 completed all native AArch64 compilation and linking. CTest
registered 1,436 tests and reported zero failures: six tests were disabled
upstream, three were skipped, and 1,427 ran successfully in the build chroot.
Desktop and AppStream validation passed. This supersedes the earlier statement
that native compilation and the test suite were still unverified.

The final RPM file-processing phase then rejected `%license` because the
source-relative main/kdtree licenses were mixed with absolute `%{SOURCE2}`.
The same error is reproducible in a small real RPM build without recompiling
FreeCAD. Release 3 stages the already checksum-verified JSON license in the
source directory and lists all three paths in the relative copy form. A real
RPM regression fixture uses the family's license list and checks all three
license flags and the JSON license digest. Full RPM publication and device
validation remain unperformed until a corrected native build succeeds.

## Channel boundary and next validation

FreeCAD is a new optional package only for unstable. Its dedicated family is
`packaging/families/freecad`, with all source, spec, checks and source-generation
rules kept together. It is outside the old 25-source channel-parity snapshot
and is not registered in the automatic `uke-*` family build hooks. The user
subsequently allowed FreeCAD to accompany the initial channel synchronization
on 2 November; that supersedes the initial hold before the final reforks.
Future automatic builds target unstable only; later promotions remain gated
by test results. Submit only `fedora-rawhide-aarch64`, inspect
native build/test logs, then solve the published RPM transaction against the
official Rawhide and unstable repositories. Device installation and GUI/CAD
testing remain unperformed until actual tablet evidence is available.

## Stable-source tracking and pinned dispatch

The user requested automatic latest-version builds. The implementation follows
the upstream latest stable release, rejects drafts/prereleases/regressions and
checks both published source digests before source preparation. Candidate pins
are isolated until the RPM source preparation and official Rawhide AArch64 SAT
transaction succeed. Synthetic fixtures confirm that failures do not advance
the real source pins and that unchanged releases are idempotent.

The persistent COPR package was initially pinned to a fixed source commit.
A generic custom webhook would keep using that old definition. Use COPR's
[SCM build API](https://github.com/fedora-copr/copr/blob/main/python/copr/v3/proxies/build.py)
with the validated `committish` in every request instead. The request is limited
to the unstable project and AArch64 chroot. Build history includes source-stage
failures without package metadata; inspect their source configurations as well
as normal named builds before submitting. A truncated history or uncertain POST
must not become permission for a duplicate request.

The current cloud runtime has no COPR API credential binding. The workflow
requires existing credentials in two named GitHub Actions secrets and reports
missing configuration before any pin update. No credentials are generated or
exposed. Publication to main and actual automatic dispatch remain unverified
until the reviewed changes are merged, credentials are configured and a real
workflow run succeeds. Local tests and a real unchanged-release provenance
check do not establish that automatic COPR access is enabled.

## Uke RPM names and repository include filtering, 3 November 2026

Native build 11247905 published signed `freecad` and `freecad-data` release-3
RPMs. Their signatures, ELF64 AArch64 executables, repository checksums and a
508-package official-Rawhide/unstable dependency transaction passed. The device
repository later supplied `includepkgs=uke-*,senemos-uke-linux-kernel-*` as a
required policy. DNF removes unmatching real package names before dependency
resolution; a compatibility provide cannot make a filtered RPM visible.

Release 4 names the real source/binary packages `uke-freecad` and
`uke-freecad-data`. The freecad family directory, upstream source pins, commands,
desktop identity, private libraries and data paths retain their existing names.
The main package requires the real Uke data package at the identical epoch,
version and release. Versioned compatibility provides satisfy existing consumers;
obsoletes cover older/equal unprefixed packages and conflicts cover newer ones.
The existing epoch 1 is preserved for upgrades.

The real RPM/DNF fixture test takes these relationships from the expanded spec.
It contains the legacy RPMs in the same local repository and applies the exact
include filter, then checks installation, compatibility consumers, upgrades and
newer-upstream conflicts. Locally generated unsigned fixtures seed only a
temporary RPM database with digest verification and no payload or scripts.
That fixture policy does not change published COPR signature requirements.
The native renamed build and its signed, filtered dependency transaction must
be verified separately. No device filter or tablet installation is changed.
On Fedora 44 and Rawhide host containers, the source contracts, license RPM
fixture, automation fixtures and exact-filter DNF tests passed. DNF used
`--forcearch=aarch64`; no fixture application payload was installed. Source RPM
generation and all four zero-fuzz patches also passed under the Uke source name.

Future dispatch checks both `uke-freecad` and legacy `freecad` build history.
An active legacy build still prevents a concurrent request, while an older
successful unprefixed release does not suppress the Uke-named build. Automatic
main publication and credential activation remain subject to their existing hold.
