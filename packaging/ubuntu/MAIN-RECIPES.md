# Canonical main source recipes

1 November 2026: the speaker-preset route now has canonical data inputs under
its owning firmware component. Fedora speaker-srpm and Debian packaging use
the same preset and original notice, checked by one shared host-only auditor.
The active Launchpad recipe now uses this main layout.

The [main recipe](launchpad-main-recipes/uke-speaker-firmware.recipe) selects
only that component's source inputs, metadata and generated Debian overlay from
main. Its fixed base is the existing docs-only ancestor c762763, so unrelated
components do not enter the source package. Source, metadata and packaging
must resolve to one identical main commit; a mixed revision is refused during
source clean and binary configuration. The version tracks revtime:source.

Canonical release metadata remains in recipes/uke-speaker-firmware/debian.
prepare-main-overlay.sh derives the snapshot overlay from it and the shared
auditor; tests/main-recipes.sh refuses stale generated files. Release source
assembly still uses the original quilt layout and original source pins.
Main snapshots use 3.0 native source format without a pristine-tar branch.

The snapshot version 3.0.304.0+uke0ubuntu2~tenacious1 upgrades the existing
conventional 3.0.304.0-0ubuntu1~tenacious1 family, but also outranks conventional
Debian revisions at that same upstream version. Preserve this integration
version scheme when promoting a qualified snapshot; do not silently switch
back to a lower conventional revision. Upstream source remains 3.0.304.0.

Static input, generated-overlay and refusal checks pass. Clean Tenacious
ARM64 replacement source and binary qualification passed on reconciled
commit `e76e45643df83425c63f3127a1e6141510e322d5`, preserving main's TUI/GTK/Qt
changes through `a9428ca`. The exact preset, notice, provenance and license
read back unchanged; no Python payload or runtime dependency entered the DEB.
The native version upgrades the published conventional package.

The replacement also passed actual Launchpad [source construction4108876](https://launchpad.net/~mcs45tr/+archive/ubuntu/uke-linux-unstable/+recipebuild/4108876)
and [ARM64 build33899943](https://launchpad.net/~mcs45tr/+archive/ubuntu/uke-linux-unstable/+build/33899943).
All three nested inputs resolve to20c7d841ac493c0453b2b3a525a372ae729d1411.
The preset, notice, attribution and license read back unchanged from the actual
DEB. Its conffile survives a disposable data-package upgrade and removal; purge
removes it. That lifecycle test registered ARM64 as a foreign architecture in
an Ubuntu24 x86_64 container; no target program or maintainer helper ran.

At21:53 UTC, the source publication remains Pending. A successful build alone
does not admit branch deletion: verify the new source and binary through the
PPA's signed APT index, exact version ordering and an isolated upgrade solver
before removing archived heads. Preserve the existing recipe during this wait.

This does not admit an image,
prove speaker operation, grant rights for other firmware or enroll additional
recipes. Keep the existing recipe and its branches until the replacement is
qualified and the publication transition is authorized. Archive unique source
and pristine metadata tips before deleting old branches.

## Additional adapters

DisplayPort/UCM, speaker-control data, GPU defaults, Bluetooth and speaker
orientation passed clean native Tenacious ARM64 source/package construction
and payload audits atdcbf8288afe4456f32a95d1fc3fca56342d54d2f. The core
selection profile, front-color and proximity capture passed at94f3f26e15d63c6e6dba752eeaf6e4b96e284831.
Downloaded artifact manifests and the three equal input revisions verify
independently. The complete owning components compare unchanged to the approved
baseline. Installed data and both UCM links match the shared source.

All eight routes remain unenrolled in Launchpad. Their real runtime ABI
requirements, false qualification flags, opt-in service policy and root-only
diagnostic mode remain unchanged. Package construction does not qualify service
or device operation.

An installation classifier main adapter now passes source construction,
same-revision and static dependency/version checks. It preserves the original
actual-Uke, container and chroot guards; native qualification remains pending.
Nine other historical non-kernel routes still need main adapters, including
explicit battery release and touchscreen input reconciliation. Historical
source exports remain reproducible through archived tags.
