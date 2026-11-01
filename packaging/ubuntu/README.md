# Ubuntu shared-source package migration

Recorded 1 November 2026. Target: Ubuntu 27.04 **Tenacious ARM64**, unstable
only. Migration is incomplete and no package is publication-ready. Fedora RPM
recipes and common C/C++ sources are unchanged. Debian overlays select the same
reviewed source/patches; there is no RPM conversion or separate application fork.

Launchpad's [series](https://api.launchpad.net/1.0/ubuntu/tenacious) and
[ARM64 architecture](https://api.launchpad.net/1.0/ubuntu/tenacious/arm64) are
available. Parent confirmed accepted PPA terms and public ARM64-only
`mcs45tr/uke-linux-unstable` and `mcs45tr/uke-linux-stable`, both empty. Stable
remains gated. Parent confirmed OpenPGP enrollment on 1 November; the private
key remains on the user's tablet. Local signing and uploads await package
qualification. Native server builds of the prepared source set were approved
on 1 November, including build helpers and the disposable kernel module key.
The dispatchable `Ubuntu Tenacious native ARM64 packages` workflow bootstraps
a clean ARM64 root from the signed official Tenacious archive. Builds run as
an unprivileged user; artifacts include source, DEBs, build logs and buildinfo.
SensorProxy first builds and installs the exact patched libssc in its own
disposable server environment. No tablet execution is involved.
No key, credential, upload or Ubuntu image was created here.

Test-image work has now resumed on explicit user request. The experimental
test-channel 1006 source is staged and the coordinated kernel suffix is fixed;
its new native source execution and complete boot cohort remain unqualified.
The official Ubuntu/PPA userspace qualification runs independently and exports
evidence only. See [TEST-IMAGE-GATES.md](TEST-IMAGE-GATES.md).

## Implemented source routes

The [APT/dpkg backend policy](APT-BACKEND.md) now has read-only native tests for
managed sources, interrupted package states and transaction exclusions. Its
solver, authenticated metadata, journal and boot integration remain unfinished;
the Fedora backend is still active and no Ubuntu Control capability is claimed.

`components.tsv` lists **20** prepared Debian source recipes, defining **30**
binary packages. Candidate packages are not deployable kernel packages.

| Source | Implemented scope | Validation |
| --- | --- | --- |
| uke-battery-observer | Collector plus separate gated charge-control and offline-charge binaries | Source only |
| uke-front-color | Opt-in snapshot service; SSC lifecycle binding | Source only |
| uke-proximity-capture | Root-only bounded diagnostic | Source only |
| uke-ssc-fileservice | Six shared FastRPC patches, cache, service ordering, udev and refresh triggers | Source only |
| libssc | Seven shared patches; original Ubuntu binary names and GIR split | Source only |
| iio-sensor-proxy | Two shared patches; SSC, Polkit and systemd interfaces | Source only |
| upower | Five shared patches, original Ubuntu binary names and lifecycle | Source only |
| uke-touchscreen | Verified THP import with four shared geometry/tracking patches | Source only |
| uke-vaapi-v4l2 | Six shared codec/capability patches; upstream Debian recipe inspected | Source only |
| uke-bluetooth | Guarded factory address helper, udev and service | Source only |
| uke-speaker-orientation | Existing Wayland/ALSA helper and desktop autostart | Source only |
| uke-installation-profile | Existing root classifier and guarded configure reconciliation | Source only |
| uke-displayport-audio | Seven UCM files, two links, WirePlumber policy | Data-only DEB/payload checked |
| uke-speaker-controls | Original KMix desktop launcher/profile | Data-only DEB/payload checked |
| uke-gpu-defaults | Root-stage MSM module/options with explicit kernel/firmware gates | Data-only DEB/payload checked |
| uke-core-meta | Ubuntu dependency graph and unqualified profile | Data-only DEB/payload checked |
| uke-orangefox-recovery | Existing Android v003 artifacts, exact checksums and source/license records; no install hooks | Data-only delivery, not recovery compilation |
| uke-image-tools | Six original native image/payload tools from the shared builder source | Source only; no system image composition |
| uke-linux-kernel | Exact experimental release-1006 source composition; coordinated candidate Image, DT and modules | Source/signature/chunk identities checked; new native execution and full boot integration pending |
| uke-speaker-firmware | Exact separately attributed FS19xx preset, OEM notice and conffile preservation | Data-only DEB/payload checked |

Native server results are recorded in [NATIVE-BUILDS.md](NATIVE-BUILDS.md) and
`native-build-results.json`; source-only results below predate those builds.
These recipes use shared
component baseline `cbecc441059ebffa97497e736edd2e0b0b0546a5`, except battery
1.4.1, which uses the exact released source pinned in `release-sources.tsv`.
Source RPM extraction retrieves only the named, checksum-verified source tar;
it neither converts a binary RPM nor executes RPM specs. This preserves the
mandatory integrated protection inventory proof missing in the initial 1.2.0
baseline. SSC release 26's cache and matching test source are pinned separately
in `release-overrides.tsv`, preserving its verified OEM-reference selection
and packaged fallback. Its six patches and five service/udev files match the
common baseline. Later integration patches for other components need review.
Front-color's Fedora wrapper version 1.0.2 contains application source 0.1.0;
its five C++ source/header files match the shared baseline byte for byte.
No installed/boot/device result
is inferred from source assembly or a data-only package build.

Service activation follows the original choices: front-color remains opt-in;
battery/touch-independent boot services retain boot enablement, with no forced
start during package configuration. D-Bus, udev and desktop event activation
are retained. SSC refresh triggers act only on an already active Uke registry.
Installation-profile reconciliation remains guarded against non-Uke, chroot
and container hosts. Private touch inputs are excluded from public preparation.

## Reproduce packaging checks

```sh
bash packaging/ubuntu/fetch-upstreams.sh
bash packaging/ubuntu/fetch-recovery.sh
bash packaging/ubuntu/tests/source.sh
bash packaging/ubuntu/tests/cohort.sh
bash packaging/ubuntu/tests/data-packages.sh
bash packaging/ubuntu/tests/apt-policy.sh
bash packaging/ubuntu/prepare-source.sh libssc /tmp/libssc-source-new
```

Use Bash, Git, curl, jq, tar/xz, libarchive-tools and dpkg-dev; data-only builds also need official
debhelper and its dependencies. Downloads are source bytes checked against
`upstreams.tsv` and release manifests; no downloaded code is executed.
Recovery downloads existing Android artifacts as opaque delivery data, not
Ubuntu image inputs. Archives live under the owning
component's `referances/`. Existing upstream Debian metadata is retained under
`uke/upstream-debian` for review before applying the active overlay.

Source preparation emits original and Debian tarballs, unsigned `.dsc`, source
`.changes` including original sources, SHA256SUMS and source-origin records.
The output directory must not exist. Debian quilt applies the same 26 shared
quilt patches used in five external source routes, plus four touchscreen patches. No build commands, keys or
uploads run in source preparation. Data-only tests build four configuration
packages, recovery delivery and the attributed speaker preset; they do not
compile or run upstream programs.
The recovery ZIP bypasses normalization so its published checksum remains exact.

Host evidence: Debian 13 x86_64, dpkg 1.22.22 and official Ubuntu debhelper
14.5ubuntu1; target data-package metadata is ARM64. Dependency checks were
skipped for these isolated data-only builds, so successful packaging does not
mean the dependencies are installable. CI uses official Ubuntu source tools
and performs no upstream native compilation, hardware execution or publication.

## Coverage and remaining work

`coverage.json` accounts for all **43** COPR source entries, including historical
standalone entries. `local-specs.tsv` records **30** literal RPM spec paths;
`components.tsv` is the authoritative implemented Debian recipe list. Source
entries and binary packages are not interchangeable counts. Partial integration
entries remain partial even when some child recipes pass source checks.

See [actual Tenacious dependency evidence](DEPENDENCIES.md) and the selected
version inventory. PipeWire, KDE/DDC and firmware/kernel ABI gaps are explicit.
Ubuntu libstdc++6 replaces the Fedora alias; stock library candidates still need
patch comparison. Original KDE applications are not forked. Desktop repository
migration is handled separately.

See the [kernel candidate boundary](KERNEL-SOURCE.md),
[firmware provenance decisions](FIRMWARE-PROVENANCE.md), and explicit
`publication-gates.json`. The kernel recipe does not emit a usable boot bundle.

Implementation still needed: Control's APT/dpkg transaction/channel backend;
current source-family delta reconciliation and power-family release alignment;
PipeWire/Mesa/remaining firmware/OEM resources; fresh recovery source-build validation;
remaining video/session and
metapackage payloads. The absent generated THP directory was recovered from its recorded commit
34046210932d654a4c0df0121ecc31c008f8148c with exact input and transformed-output
hashes. No donor executable was run; native touchscreen build and current
integration-release reconciliation remain pending.
These are implementation gaps, not signing blockers.

[Boot cohort checks](BOOT-COHORT.md) reject mismatched artifact identities but
are not a finished initramfs or kernel port. Real kernel/DT/module validation,
initramfs-tools integration, upgrade/rollback and device acceptance remain.
Native ARM64 server builds are authorized. Successful compilation must still
be followed by install/upgrade/remove and transitive dependency checks.
Source signing and Launchpad publication follow only after those gates.
