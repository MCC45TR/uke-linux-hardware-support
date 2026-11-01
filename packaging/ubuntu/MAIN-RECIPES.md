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

The replacement also passed actual Launchpad [source construction 4108876](https://launchpad.net/~mcs45tr/+archive/ubuntu/uke-linux-unstable/+recipebuild/4108876)
and [ARM64 build 33899943](https://launchpad.net/~mcs45tr/+archive/ubuntu/uke-linux-unstable/+build/33899943).
All three nested inputs resolve to `20c7d841ac493c0453b2b3a525a372ae729d1411`.
The preset, notice, attribution and license read back unchanged from the actual
DEB. Its conffile survives a disposable data-package upgrade and removal; purge
removes it. That lifecycle test registered ARM64 as a foreign architecture in
an Ubuntu24 x86_64 container; no target program or maintainer helper ran.

Launchpad published the new source at 22:05:34 UTC. Subsequent verification
passed the complete PPA-key/InRelease/SHA512-index/source/ARM64-DEB chain.
The authenticated DEB matches the actual build output byte for byte. An
isolated APT solver selects the exact new version as an upgrade from the earlier
published conventional version, with no package removal or downgrade. The
replacement recipe's publication gate has passed; preserve its main inputs
and the archived historical tips while removing obsolete heads.

This does not admit an image,
prove speaker operation, grant rights for other firmware or enroll additional
recipes. Keep the existing recipe and its branches until the replacement is
qualified and the publication transition is authorized. Archive unique source
and pristine metadata tips before deleting old branches.

## Additional adapters

DisplayPort/UCM, speaker-control data, GPU defaults, Bluetooth and speaker
orientation passed clean native Tenacious ARM64 source/package construction
and payload audits at `dcbf8288afe4456f32a95d1fc3fca56342d54d2f`. The core
selection profile, front-color and proximity capture passed at `94f3f26e15d63c6e6dba752eeaf6e4b96e284831`.
Downloaded artifact manifests and the three equal input revisions verify
independently. The complete owning components compare unchanged to the approved
baseline. Installed data and both UCM links match the shared source.

All eight routes remain unenrolled in Launchpad. Their real runtime ABI
requirements, false qualification flags, opt-in service policy and root-only
diagnostic mode remain unchanged. Package construction does not qualify service
or device operation.

The installation classifier also passed native Tenacious ARM64 qualification
at `4600858f32a8c704c0120c6e32bc11a5f6d87eaa`. Its downloaded artifact hashes,
same-revision source manifest, AArch64 payload and actual-Uke/container/chroot
maintainer guards verify independently. It remains unenrolled in Launchpad.

The libssc adapter retains Fedora's upstream archive and seven shared patches.
Every unpacked upstream file matches the historical quilt result. Its clean
native Tenacious ARM64 build passed at `d0d453a31772b821b08a3b50b5ff3d29520e6f18`;
all four downloaded binary packages, artifact hashes and input revisions verify.
The library retains the real dual-light ABI; its development package remains
a host SDK input. No new Launchpad enrollment or sensor operation is claimed.
Source fixtures reject corruption, incomplete or broken patches and leave no
partially prepared tree.

Eight other historical non-kernel routes still need main adapters, including
battery release and touchscreen input reconciliation. Historical source exports
remain reproducible through archived tags.

## Completion of the nineteen non-kernel main routes

1 November 2026, superseding the remaining-adapter statements above: all eight
remaining routes passed clean native Tenacious ARM64 source/package construction
at `23e25a7fc4f442d30cd13bbcbfa9478f2e11f782`: battery observer, touchscreen,
SSC fileservice, SensorProxy, UPower, VA-API, recovery delivery and image tools.
Downloaded artifact/manifest hashes, all three equal input commits and direct
payload audits pass for all thirteen resulting binary packages. Every prepared
upstream source file matches the earlier approved native source set. The same
job built the real matching libssc SDK before compiling SensorProxy.

All nineteen non-kernel routes now have canonical main adapters; the eighteen
additional recipes remain unenrolled in Launchpad. SensorProxy's follow-up
build ABI requirement is satisfied by the real SDK recorded in its native
builder; live source builds require libssc to be published in the PPA first.
The SSC RPM release advances beyond the archived26 family to avoid reusing an
older published revision for the reconciled cache code. These follow-up changes
are metadata; compiled code, flags and runtime guards are unchanged.

Recovery is unchanged previously published Android v003 delivery data with its
source offer and licenses, not an Ubuntu recovery or new image. Image tools
contain only six AArch64 inspection utilities at the approved builder pin.
Kernel/boot source integration, missing runtime firmware/ABI providers and the
remaining43-source inventory are separate unfinished work. This checkpoint
does not admit a root image or promote anything to stable.
