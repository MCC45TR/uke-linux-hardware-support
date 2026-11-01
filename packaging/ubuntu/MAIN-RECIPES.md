# Canonical main source recipes

1 November 2026: the speaker-preset route now has canonical data inputs under
its owning firmware component. Fedora speaker-srpm and Debian packaging use
the same preset and original notice, checked by one shared host-only auditor.
The existing active Launchpad recipe has not been changed.

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

This is private server qualification; the replacement has not yet run through
Launchpad's source and ARM64 binary builders. The existing public daily recipe
still uses its original branch. After importing the reconciled main commit,
replace that recipe with the checked text above, build it for Tenacious only
and verify source acceptance, ARM64 binary publication and APT upgrade order.
Only then archive and remove the old active source branch. Other historical
source routes remain pinned snapshots and still need canonical-main adapters.

This does not admit an image,
prove speaker operation, grant rights for other firmware or enroll additional
recipes. Keep the existing recipe and its branches until the replacement is
qualified and the publication transition is authorized. Archive unique source
and pristine metadata tips before deleting old branches.

## Additional adapters

DisplayPort/UCM, speaker-control data and GPU defaults now have generated main
overlays. Official Ubuntu24 x86_64 tools constructed source and ARM64-tagged
data-only DEBs with dependencies checked; installed bytes and both UCM links
match the common source. They retain all missing runtime ABI requirements.
This is not native Tenacious or service/device admission.

Bluetooth and speaker orientation have source-only main adapters with guarded
input revisions and prefixed shared source paths. Source construction and
dry build plans pass; no helper compilation or execution occurred in this
adapter qualification. These five routes remain unenrolled in Launchpad.
The other thirteen historical non-kernel routes still need main adapters.
