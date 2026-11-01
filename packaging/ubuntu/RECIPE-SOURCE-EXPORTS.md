# Shared-source recipe exports

## Main-only migration update

The active speaker daily recipe now uses the canonical main recipe described
in [MAIN-RECIPES.md](MAIN-RECIPES.md). Historical export and pristine metadata
tips are preserved as `archive/main-only-20261101/launchpad/PACKAGE` and
`archive/main-only-20261101/pristine-tar` tags. Before removing old heads, the
replacement's actual Launchpad source, ARM64 binary and signed APT checks
must pass. The older branch-based enrollment instructions below are historical.

`prepare-archived-recipe.sh PACKAGE NEW_DIRECTORY` materializes a disposable
local repository and recipe from those exact tags. Source and metadata pins
are checked against `recipe-source-manifest.tsv`; original archives retain
their recorded hashes. All nineteen exports reconstructed byte for byte using
official recipe tools without network or upstream execution. This preserves
historical reproducibility without requiring remote export/pristine branches.
It is not an automatic daily main adapter for the other eighteen packages.

Recorded 1 November 2026. Nineteen non-kernel source routes passed native
Tenacious ARM64 package construction and now have public `launchpad/PACKAGE`
branches in the shared hardware-support repository. The kernel retry remains
active; no kernel recipe branch is admitted yet. Recipe preparation is not
Launchpad publication or device qualification.

[Recipe files](launchpad-recipes/) use the actual project-scoped import URL:
`https://git.launchpad.net/~mcs45tr/uke-linux-hardware-support/+git/uke-linux-hardware-support`.
The parent confirmed a reviewed, queued import; source recipes must wait for
its required branches to become visible. Tenacious and ARM64-only unstable are
the target. Stable has no automatic recipe.

## Source identity

`prepare-recipe-source.sh` extracts a checksum-verified native-build DSC with
patches unapplied, records an upstream tree, adds its unchanged Debian overlay
and preserves the exact original archive with official pristine-tar. This
avoids applying the shared patches twice. Package-specific `source/PACKAGE/VERSION`
tags avoid collisions between unrelated components at version 0.1.0; tilde is
encoded as underscore in Git tag names only. Debian versions retain the tilde.

The common `pristine-tar` branch at
`eab577e70d968731985a0e87366ef28b441010cf` reconstructs all nineteen original
archives byte for byte, including the first speaker preset export. Preserve
all package branches and this metadata branch in the import. Metadata commits
reference source trees reachable from those package branches. Do not import
only one branch and discard the other source objects.

[recipe-source-manifest.tsv](recipe-source-manifest.tsv) records branch and
original-archive identities. The kernel entry is deliberately absent. Recipe
versions use `{debversion}+git{revtime}`, preserving the shared patched Debian
revision and satisfying the dependency-first libssc threshold.

## Validation

Each additional export passed official git-build-recipe 0.3.7 source assembly
with `--safe --no-build`, exact original-archive comparison and dpkg-source
construction. These source-only checks ran in a disposable official Ubuntu
24.04 x86_64 tool container. They do not replace the separately completed
Tenacious ARM64 native builds. Full source-build invocation on this older host
correctly refused unsatisfied Tenacious build dependencies; source-only checks
preserve those requirements instead of weakening them. The tool's upstream
Python dependencies remain host-only; this is the service's required recipe
implementation, not project-owned Python or a tablet runtime.

The regression fixture creates two unrelated 0.1.0 source packages, supplies an
ambiguous generic upstream tag and checks that both exact original archives
survive shared-repository import. Source CI runs this test. The dependency-first
libssc and SensorProxy pair was also reconstructed from the combined repository.
Recovery delivery retains the existing Android artifacts; no new recovery or
Ubuntu system image is created. Its existing 100 MiB file was accepted by GitHub
with a size recommendation warning. No Git LFS or persistent credential was added.

## Enrollment gates

Start with the independently attributed speaker preset. Publish patched libssc
before SensorProxy so the source build can resolve its actual libssc-dev gate.
Image tools, Bluetooth, VA-API, installation profile and UPower have resolved
runtime metadata, but service lifecycle and hardware acceptance are separate
from package construction. Battery observer resolves separately; its charge
control and offline-charge binaries remain gated on missing kernel/boot ABIs.
Do not automatically enroll the entire battery source group yet.

[runtime-dependency-results.tsv](runtime-dependency-results.tsv) records all 27
built binary packages. Official APT simulation using the supplied Tenacious
indexes plus metadata from those actual checksum-verified DEBs resolves 16
closures and rejects 11. Fourteen resolved closures have no Python in the plan;
the two development packages have host Python dependencies and are server SDK
inputs only. Exclude them from tablet images. No fake provider was inserted.
This is a solver fixture, not an authenticated repository or runtime install.

Hold charge control, offline charge, core meta, DisplayPort audio, front color,
GPU defaults, proximity capture, speaker controls, speaker orientation, SSC
fileservice and touchscreen until their real kernel, boot, firmware or audio
providers exist. Missing providers are implementation work. Do not lower ABI
thresholds, fabricate Provides, suppress safety checks or infer hardware support
from their successful native compilation. Remaining ports and complete firmware
redistribution review remain tracked in [coverage.json](coverage.json).
