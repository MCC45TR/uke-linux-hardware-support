# UKE-SLOT-SELECT-20261104: select an existing verified image through the boot owner

Date: 4 November 2026. Environment: Fedora 46 Rawhide aarch64 on POCO Pad X1,
running kernel 7.3.1-1500. Native builds run only in COPR.

The manual selection API extends the exact published 1.8.39 boot integration,
source d03d854032dd7c0b011ac1be2ff1089850af0403, COPR build 11251582.
Its SRPM SHA-256 is 2d553e6ea908e0a66b63090b5a5f04e59a44cccf12e0dca16a827888d29011db.
The source recipe verifies the retained archive inventories and changes only
the kernel updater entry point, new selection helpers, their contract tests and
package metadata. RTC, touch health, firmware qualification, inactive-image
protection, existing services and installation scripts remain from that cohort.

Selection uses the established Uke QTI slot implementation in slot-control.hpp,
with its Lineage bootctrl 25092672 / OpenABL aef4e86045bd attribution. The workspace
donor policy and acquisition inventory remain research input. This adds no
hardware port, donor geometry, early-firmware writer or second GPT implementation.
The Uke updater already owns paired GPT type/flag updates, UFS boot selection,
locking, readback and restoration; Control delegates to this owner.

The preflight proves the running slot, recorded accepted image, next selection,
paired GPTs and early firmware. A changed target must match an installed,
RPM-verified kernel/module bundle and the current userdata wrapper policy.
The plan distinguishes a new trial, cancellation before its first boot, return
from a failed trial, rollback to an accepted kernel and an unchanged selection.
A previously failed release and incomplete/current-slot trials remain refused.

Applying changes only the existing approval record, GPT metadata and UFS next
boot attribute, under the original shutdown inhibitor and critical-signal guard.
Both boot images are read back unchanged. Metadata uses the existing small GPT
transaction recovery records; no full-image backup or partition copy is created.
Any stage failure restores the original record, GPTs and UFS selection. Selection
never requests reboot or replaces the normal post-boot health acceptance.
It chooses the next boot; later package policy may stage another kernel.

Pure contract tests cover no-op, trial, cancellation, failed return, rollback,
inconsistent selection, failed/same release and incomplete phases. Injected
record/GPT/UFS/readback failures require restoration; restoration failure reports
recovery explicitly. Build and device results will be recorded after observation.

## Observed delivery and limits

COPR [11259613](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11259613/)
built source df2609c05674a33a7dba561daf18e09e77b3f090 as 1.8.40-1.fc46. The existing
GPT/slot, trial-health, RTC and touch checks passed, together with the new
existing-slot selection and injected-failure transaction contracts. The custom
package producer was pinned to that source recipe after successful publication.

On 4 November, DNF upgraded only boot integration, kernel updater and installation
profile from 1.8.39 to 1.8.40. No kernel-image package was included. The retained
postinstall reported the existing 7.3.1-1500 userdata bundle and disabled automatic
boot activation policy. The running kernel remained 7.3.1-1500, UFS next boot
remained B, and the approval record remained stable with no candidate.

With Control 2.8.1-12, real B preview and already-selected B selection returned
success without a change. Both A and `next` previews resolved the stored
7.3.1-7008-unstable image and refused its recorded previous failure. Invalid
targets, JSON/automatic confirmation and status transaction flags were rejected.
No real nontrivial slot transition or reboot was performed: A is a failed image,
so the guarded refusal was preserved. Trial/cancellation/rollback write-path
results remain remote synthetic evidence, not tablet boot acceptance.
