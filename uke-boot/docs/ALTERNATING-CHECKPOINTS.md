# Alternating updates with rollback disabled

5 November2026. Source candidate; native COPR and device acceptance pending.
The update extends the exact complete1.8.97 source capsule from build11264107,
SHA2564ddde3381fe3c1afa3e3c70b054346214c291618d9cc93fb9c9d70d5b805b2c7.
All firmware, color, RTC, slot-identity, root-profile and transaction checks remain.

Automatic updates stage only into `1-running_slot`, including rollback-off.
A booted trial can become an explicitly unapproved development checkpoint for
the next alternating transaction. This does not run health acceptance, change
rollback.conf, promote a stable kernel, or call the former current-A replacement
path. Existing unbooted trials remain pending until reboot. Failed candidates,
downgrades and ambiguous image/slot identity retain their refusal.

Format2 records mark `baseline_approved=0`; format1 remains the canonical
approved-baseline form. Bootloader retention and health approval are separate:
the prior running checkpoint's bootloader flag keeps it usable if boot firmware
needs it, while update status continues to report it unapproved. An unapproved
checkpoint cannot authorize automatic health rollback under enforced policy.
A later genuine health acceptance uses the existing qualification path.

Before reusing the physical accepted slot, retain its exact kernel Image, small
boot header and protected approval record as required recovery components. The
accepted full boot hash is checked against the protected record before extraction.
Existing components must match exactly; all files are protected and synced.
Neither a root-filesystem image nor a full boot-partition backup is created.
The last running kernel's installed Image, modules, firmware and bundle remain
the immediate fallback. Retained older kernel components do not assert that its
old modular drivers are still installed. Missing old modules remain an explicit
recovery limitation; this never claims a complete old-package rollback.

The inherited inactive-image/GPT/UFS transaction performs writes and readback.
Preview creates no recovery artifacts or slot/approval changes. Package defaults
remain automatic staging enabled and rollback enforced, preserving the local
rollback-off preference with config(noreplace). Control26 understands the
unapproved record; the updater conflicts with older Control readers.

COPR tests cover both starting slots, repeated alternation, canonical state
round trips, incorrect boot/slot/hash/policy rejection, preview GPT immutability,
separate bootloader retention, small synthetic recovery artifacts and inherited
firmware/transaction fault controls. Actual alternating installation, reboot,
automatic conversation continuation and hardware acceptance remain required.

After an interrupted inactive write/verification, the off-mode acceptance path
can restore a stable checkpoint record only after proving the unchanged running
baseline and current GPT/UFS selection. This performs no health acceptance or
slot change. A later boot that returned to the baseline retains the failed
candidate marker, so it cannot silently retry that version. Tests cover these
interruption phases and enforce unchanged caller state during planning.
