# UKE-BOOT-STATUS-20261104: distinguish running, selected and accepted kernels

Date: 4 November 2026.
Environment: Fedora 46 Rawhide aarch64 on POCO Pad X1 (xiaomi,uke), running
kernel 7.3.1-1500. Native compilation is restricted to COPR.

## Source and device evidence

At initial inspection, the installed `uke-kernel-update` was 1.8.39. Its producer pinned source
`d03d854032dd7c0b011ac1be2ff1089850af0403`; earlier kernel-helper sources must not
replace its retained RTC, touch-health, rollback and inactive-image safeguards.
The existing QTI flag contract is in `uke-boot/src/slot-control.hpp`; the
userdata approval record is in `src/userdata-update.hpp`. Status reuses those
meanings without linking any writer.

On the tablet, the bootloader provides no slot suffix in the command line.
Consequently, the active flag or UFS next-boot choice cannot establish the
currently running slot. The report identifies it only through an explicit
consistent slot suffix or a unique match against the installed running kernel
Image. It reads each Android v4 slot image directly, retaining hashes only in
memory, and checks next-boot UFS selection against GPT flags. Missing,
ambiguous or conflicting evidence stays unknown.

The private approval record names slot B and release 7.3.1-1500 as stable, while
also retaining an older failed-candidate field. The record has no ordered attempt
history, and acceptance does not clear that history field. A status command must
therefore distinguish verified current/accepted kernel approval from an
unprovable claim about the latest historical attempt. It reports the previous
failure separately and leaves that chronological question unknown when needed.

## Interface and boundaries

`uke-control update status` prints a human-readable summary by default. The
versioned JSON mode is for the Qt adapter. A dedicated helper opens only fixed
status sources read-only, takes a shared existing updater lock without creating
state, streams bounded images and omits hashes, partition UUIDs, boot identities
and private operation IDs from its output. It does not repair GPT, select slots,
run RPM or reboot. Active local sessions may request only this fixed operation
through PolicyKit; other sessions require administrator authentication.

Source tests cover slot ambiguity, pending trials, UFS/GPT disagreement,
acceptance mismatches, missing records, retained failure history, parser refusal,
bounded boot-image parsing and output privacy. Build, installation and device
results will be recorded after observation. Slot-selection work remains separate
and must use the current guarded boot helper rather than a second raw writer.

COPR [11259624](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11259624/)
passed the backend package checks and produced 2.8.1-12. DNF upgraded only
`uke-control` and its client. Root status and a normal desktop-manager process
both returned complete evidence: B is running and selected, 7.3.1-1500 is accepted,
A holds 7.3.1-7008-unstable and is unbootable, and no switch is pending. Historical
attempt order remains unknown as designed. The remote agent process has no
controlling terminal/local session and requires sudo; it does not receive the
desktop's PolicyKit grant. A service launched by the desktop user manager reads
the fixed status helper successfully without administrator authentication.

With boot owner 1.8.40, B preview and the actual already-selected B no-op passed.
A and `next` previews refused the recorded failed release. No real slot change,
reboot or boot-image write was requested. The underlying trial/cancellation/
rollback transactions were exercised only by remote contract fixtures.

Release 2.8.1-13 distinguishes a known empty candidate/failure record from missing
evidence in terminal text, and no longer advertises unsupported JSON on slot
selection help. The versioned status schema and SDK are unchanged.

COPR [11259667](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11259667/)
passed all backend/package checks and published 2.8.1-13 from
dd7d48de255ec38af1796eeba8faa2beb1489dcb. The producer now pins that successful
recipe. DNF upgraded only Control and its client. The default redirected report
remained human text, with `Pending candidate: None`, proven B/7.3.1-1500 acceptance
and no pending switch. Slot help advertised only supported options. Control,
client and Qt RPM verification passed; updater verification differed only at
the two retained customized policy configuration files, which were preserved.

Installed `update --check` and `brunch unstable --dry-run` succeeded without a
package transaction, slot selection or reboot. The final Qt 1.1.0-2 five-provider
live-read check also passed with SDK/backend 2.8.1-13. A concurrent status reader
briefly held the shared updater lock and the selection preview safely refused
its exclusive lock. Repeating B preview after the read completed passed. This
is a serialization boundary, not permission to bypass the updater lock. UFS
selection and running kernel remained B and 7.3.1-1500 throughout these checks.
