# UKE-BOOT-STATUS-20261104: distinguish running, selected and accepted kernels

Date: 4 November 2026.
Environment: Fedora 46 Rawhide aarch64 on POCO Pad X1 (xiaomi,uke), running
kernel 7.3.1-1500. Native compilation is restricted to COPR.

## Source and device evidence

The installed `uke-kernel-update` is 1.8.39. Its producer pins source
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
