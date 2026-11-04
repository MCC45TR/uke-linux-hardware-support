# UKE-CONTROL-UPDATE-20261104: expose progress and diagnose channel availability

Date: 4 November 2026. Environment: POCO Pad X1, Fedora 46 Rawhide,
AArch64, Linux 7.3.1-1500, Control 2.8.1-7 and libdnf5 5.4.6.0.
All native compilation remains in COPR. Device work uses installed RPMs and DNF.

An installed `update --check` resolved successfully. The real `update --yes`
completed with 47 same-version reinstalls and four upgrades. Repository aliases
initially differed from the canonical COPR origin IDs, causing approximately
188 MB of downloads without application progress. The following preview had
zero package actions. The guarded updater recognized the already running
selected userdata kernel and performed no slot update. This qualifies the
observed package operation, not a new boot or hardware acceptance result.

Both stable and test previews stopped at the first missing package,
`selinux-policy.noarch`. A separate target-name inventory also found missing
Control client/GUI, FreeCAD, NFC and unstable audit components. The existing
refusal protects the installed graph; suppressing it or dropping packages would
not establish a complete channel transition.

The source adds libdnf5 download/RPM callbacks and stage messages on stderr,
leaving JSON stdout machine-readable. Normal COPR chroot URLs identify aliases;
multiple aliases of one channel agree, while mixed channels and custom build
directories remain ambiguous. Plans report all absent installed components
before mutation. A complete operation now reports no recovery action.

Regression fixtures cover normal aliases, conflicting channels, custom and
lookalike URLs, stderr-only progress and the completed-operation receipt.
The retained source recipe advances the five backend/SDK RPMs to 2.8.1-9 and
overlays only reviewed package-manager sources/tests. Nested source archives
and existing maintainer scripts remain byte-for-byte retained. Native COPR
build results, signed installation and channel-transition acceptance remain
pending until separately verified; successful previews do not qualify them.
