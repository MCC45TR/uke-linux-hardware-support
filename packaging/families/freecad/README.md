# FreeCAD source family for Fedora Rawhide AArch64

This optional application is built from FreeCAD 1.1.4's upstream release source
for `mcc45tr/uke-linux-unstable`, `fedora-rawhide-aarch64` only. All FreeCAD spec,
source pins, checks and source-generation rules live in this separate `freecad`
family. Its `uke-freecad` source package produces `uke-freecad` and
`uke-freecad-data`, plus RPM-generated debug packages. Hardware-family COPR
hooks and release-image package selections are managed separately. Testing and
stable promotion require separate test results and authorization.

The real RPM names match `includepkgs=uke-*,senemos-uke-linux-kernel-*`.
Versioned `freecad`, `freecad(aarch-64)` and `freecad-data` provides retain
dependency compatibility. Older or equal-version unprefixed packages are
replaced during upgrades; newer upstream packages conflict to prevent mixed
ownership of the same application files. The `FreeCAD`, `FreeCADCmd` and
`freecad` commands, `org.freecad.FreeCAD` desktop identity, and upstream data and
configuration paths retain their existing names.

## Source provenance

The spec is adapted from Elaine Gibson's
[Fedora package review](https://bugzilla.redhat.com/show_bug.cgi?id=2386504),
with the upstream stable release updated from 1.1.3 to 1.1.4. Fedora's previous
FreeCAD package was retired after its Python 3.12 installation failure
([retirement marker](https://src.fedoraproject.org/rpms/freecad/raw/rawhide/f/dead.package)).
That retirement does not establish a current AArch64-specific failure.

`source-lock.json` records the source archive, upstream commit, published
SHA-256, upstream build/test fixes and the bundled JSON license text.
The source factory checks every downloaded file before creating the source RPM.
It does not consume an AppImage or a prebuilt application. The newer review's
external JSON/kdtree patches target main rather than the stable branch; this
package retains the stable branch's bundled headers and declares their licenses.
Bundled SMESH and zipios++ remain FreeCAD's maintained implementations.
Release 2 additionally backports upstream
[PySide 6.12 compatibility fix #33408](https://github.com/FreeCAD/FreeCAD/pull/33408).
Release 3 stages the pinned JSON license beside the other license texts so RPM
can package them together using source-tree-relative paths.
Release 4 applies the Uke RPM naming rule and upgrade relationships.

FreeCAD embeds upstream Python and imports Python workbenches at runtime. The
user explicitly requested this optional application for the Fedora root. Its
dependencies are Fedora packages; no project-owned Python helper, hardware
service, recovery component, initramfs or installer payload is introduced.

## Source build

On a Fedora host with `rpm-build`, `make`, `curl`, `jq`, `cmake-rpm-macros` and
`python3-rpm-macros`:

```sh
make -C packaging/families/freecad validate
make -C packaging/families/freecad srpm
```

For a COPR SCM build, use:

| Field | Value |
| --- | --- |
| Project | `mcc45tr/uke-linux-unstable` |
| Type | Git |
| Clone URL | `https://github.com/MCC45TR/uke-linux-hardware-support.git` |
| Committish | Reviewed immutable commit containing this directory |
| Subdirectory | `packaging/families/freecad` |
| Spec file | `freecad.spec` |
| SRPM method | `make srpm` |
| Chroot | `fedora-rawhide-aarch64` |
| Build networking | Disabled |

The root `.copr/Makefile` dispatches this source directory separately. Binary
compilation uses Fedora libraries and does not fetch dependencies from the
network. Developer tests, desktop metadata and AppStream validation run during
the RPM build and must pass before publication is called successful.

## Validation boundary

On 2 November 2026, source RPM generation, reproducibility, clean patch
application and Fedora Rawhide AArch64 build-dependency resolution were checked
in an isolated Fedora 46 host container. The dependency transaction resolved 936
packages entirely from the official Rawhide repositories. An x86_64 CMake
configuration passed with GCC 16.2.1, Python 3.15.2 and Qt/PySide 6.12.0; SWIG/Pivy
runtime compatibility passed. This is a source/configuration result. A native
COPR build, published RPM dependency resolution and tablet testing remain separate.

Native AArch64 build 11246564 subsequently completed compilation and its checks:
CTest reported zero failures, with six upstream-disabled tests and three skipped
tests. Desktop and AppStream validation passed. RPM file processing then failed
because a license list mixed relative and absolute paths. Release 3 corrects
that file-list syntax. Build 11247905 subsequently published signed unprefixed
RPMs; their executable architecture, repository checksums and Rawhide dependency
closure passed. That transaction did not include the device's Uke-only repository
filter. Release 4 addresses that newly supplied constraint; its native build,
signed publication and filtered dependency transaction remain separate checks.

The `freecad.yml` GitHub workflow checks this family's source contracts and
does not submit COPR builds. The existing `uke-*` family registry is unchanged,
so its test/stable hooks do not pick up FreeCAD. See the family's
[lesson record](docs/lessons/2026-11-02-freecad-rawhide.md) for evidence and limits.

After a successful Uke-named native build and filtered repository verification,
install with:

```sh
sudo dnf5 copr enable mcc45tr/uke-linux-unstable fedora-rawhide-aarch64
sudo dnf5 --refresh install uke-freecad
```

Review the transaction before accepting it. Launch `FreeCAD` or `freecad`.

## Latest stable release automation

The proposed `freecad-stable.yml` workflow checks the upstream latest stable
release every six hours and on manual dispatch. Drafts, prereleases and version
regressions are rejected. It resolves the release tag to an immutable commit,
compares GitHub's source-asset digest with the upstream checksum file, verifies
the downloaded archive, prepares an isolated source RPM and checks its patches
and official Rawhide AArch64 dependency transaction before changing source pins.
An unchanged release does not create a commit. A failed check leaves the pins
unchanged. A new release whose patches need review stops before publication.

COPR requests use the authenticated SCM API with the validated full Git commit
in each request. They do not depend on the persistent package definition's old
committish and do not enable build networking. Running builds, successful
matching releases and previously attempted identical commits prevent duplicate
requests. A new documentation or automation commit cannot retry unchanged inputs
from a failed build. An uncertain POST is not retried automatically. No project settings,
dependencies outside Fedora Rawhide, or channel-promotion rules are changed.

Activation requires merging the workflow and configuring the account's existing
COPR API login and token as GitHub repository Actions secrets named
`COPR_FREECAD_LOGIN` and `COPR_FREECAD_TOKEN`. Do not put their values in source,
logs, pull requests or chat. The workflow fails explicitly before changing pins
when either secret is absent. This implementation neither generates credentials
nor claims dispatch is enabled until those settings and a real run are verified.

Synthetic updater and dispatch tests run with:

```sh
make -C packaging/families/freecad test-automation
make -C packaging/families/freecad test-filtering
```

They cover stable-release idempotence, prerelease/regression/checksum refusals,
rollback after source-preparation or dependency failures, active/published/failed
build deduplication, pinned unstable requests and a single uncertain POST. They
make no external requests. Native builds and device tests remain separate.
The filter test builds local AArch64 RPM fixtures from the actual rename
metadata and runs DNF against the exact Uke include filter. It covers fresh
installation, compatibility consumers, older/equal-version upgrades and refusal
to mix a newer upstream package. Its RPM database fixtures do not install or
execute application payloads. Published RPM signature and full dependency checks
must still pass after the native build.
These local filter fixtures additionally require `dnf5` and `createrepo_c`.
