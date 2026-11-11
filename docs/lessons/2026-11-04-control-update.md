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

Verification update, 4 November 2026: this supersedes the pending build and
installation status above. [COPR build 11254211](https://copr.fedorainfracloud.org/coprs/build/11254211/)
succeeded for Rawhide AArch64 from source commit
`c16464cd9f261702f06ca34b8db5cbc3eefca0da`. Source-contract checks and the retained
native test suite, including the new package-manager regression cases, passed.
Five signed binary RPMs were published. The existing unstable source producer
now points to the same immutable recipe.

DNF upgraded only Control and its matched client library to 2.8.1-9. Installed
RPM verification was clean. A real update then reinstalled those two packages
from the canonical repository, exercising download, installation and scriptlet
callbacks. It returned zero with a valid JSON receipt. The ordinary interactive
`sudo uke-control update` route also returned zero, and subsequent previews had
zero package actions. The final operation phase is complete, its recovery action
is none, and no pending channel remains. Linux 7.3.1-1500 continued running;
the updater reported no slot update and no reboot was requested.

A temporary, restored configuration verified device-side channel inference
through both normal aliases without a saved channel record. Configuration
digests matched before and after that test. Actual `branch test --yes` and
`brunch stable --yes` attempts refused before mutation and listed all eight
missing packages: SELinux policy and targeted policy, the unstable boot-graph
audit, Control client and Qt GUI, FreeCAD and its data, and NFC/IR integration.
Configuration digests and the installed RPM inventory remained identical across
both refused transitions. A `brunch unstable --dry-run` succeeded with zero
actions. Complete stable/test transition acceptance remains open until their
compatible package graphs are published; the missing-package guard is retained.

Superseding result, 4 November 2026: Control 2.8.1-11 separates conservative
channel selection/updates from explicit exact synchronization. All six directed
normal channel transitions and repeated updates now pass on the tablet. Exact
`channel sync` retains its stricter missing-package contract. See the
[channel-policy and receipt verification record](2026-11-04-channel-policy.md).
