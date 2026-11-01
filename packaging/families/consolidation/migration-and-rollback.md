# Migration and rollback

The later [desktop package transition](fedora-return-results.md) explicitly
maps 12 Uke names to original Fedora packages. It requires the compatible
Control manager first and retains that manager for reverse plans. The source
ownership-only procedure below remains the record for the six integrations.

1 November 2026. This change moves source ownership while preserving binary
package names. It does not authorize a tablet transaction, boot write, reboot,
charge-control write or physical stimulus.

For each group, the baseline is the exact successful SRPM in
`../integrations.json`. Its archive hash is checked before extraction. No
binary is repackaged; the original build, installation and check sections run
in the native COPR worker. Each child retains its Version/Epoch and receives
a strictly higher Release. Existing compatibility Provides, Obsoletes bounds,
ISA requirements, configuration ownership and service scripts remain intact.
The new source name produces no installation-selection RPM.

Before routing future builds, verify all emitted binary names and EVRs against
the baseline, original check results, file manifests, payload hashes and
signatures. Review shared buildroot globs for sibling ownership. Exercise
fresh install and old, skipped and mixed-version upgrades with normal and
weak-dependencies-off solvers; include optional desktop removal and the minimum
core closure. Check preserved config files and enabled, disabled, masked and
active service states in isolated native workers. Solver and scriptlet fixtures
are distinct from a tablet installation.

Preserve the old signed RPM cohort, matching source archives and checksum
receipts. A reverse transaction explicitly selects the retained old EVRs and
must itself solve without removing protected kernel, boot, calibration or
private firmware packages. An old source definition remains until replacement
publication, migration checks and the applicable acceptance gates are proven.
Changing producer defaults does not install packages. Stable/test projects and
automatic rebuild settings are outside this unstable-only change.

Kernel, DTB, modules, initramfs, firmware and updater remain an exact cohort.
The native touch path requires physical contact/multitouch, display and suspend
acceptance before any fallback retirement. Preserve private reference data,
factory identity handoff, read-only persist and independent recovery artifacts.

The baseline is now independently retained in the public
`copr-consolidation-archive-20261101` release: 14 complete source RPMs and 21
original signed binaries. The integration manifest uses those archive URLs
and records original COPR URLs and build IDs. This avoids depending on a
historical COPR build after any later separately approved deletion. Debug
package identities remain optional; their generated source/build-id paths
naturally follow the new SRPM identity.

The applied routing changes only six integration recipes and the 14 superseded
standalone source scripts. The latter print their replacement and stop before
any build. Existing signed binaries remain installable from the retained
repository/archive. Restoring an original recipe from inventory.json is a
source-routing reversal; it is not a tablet rollback transaction. Future source
builds must advance the integration Release before an already known revision
can be generated again. Project build settings and automatic rebuild flags are
unchanged.
