# UKE-BOOT-IDENTITY-20261116: inspect the current kernel naming grammar

Date: 16 November 2026. Environment: actual Fedora Rawhide on POCO Pad X1,
kernel 7.3.3-28-unstable and Control21; separate development PC.

Control21 signed DNF completed successfully. Its real manual CDC call passes
the repaired RPM API boundary, then the native helper refuses with
DEBUG_NOT_AUTHORIZED. The inhibit file is absent. The actual read-only
uke-boot-status --identity reports the exact Uke match but rejects the kernel
name and exits2. Inspection of the signed21 source shows a selector supporting
only old 7.2.x names. This is a naming contract failure, not permission to
remove the identity or USB ownership gate.

Control22 calls the existing canonical unstable parser before the unchanged
legacy selector, retaining the128-byte bound. The identical parser already
ships as Source17 for other boot consumers. Only boot-status and its original
fixture adopt C++20, required by that existing header; other compiler flags,
binary ownership, full Type-C policy and saved CDC preferences remain.

A complete signed21 recipe inverse keeps all72 inputs, API output budgets,
RGB20, Motion19 and preceding integrations. Native tests run the original
legacy fixture and the actual packaged ELF, plus GCC/Clang sanitizers, against
current/future canonical names, malformed/overflowed releases, exact Uke and
SM7675 identity, and unterminated/foreign device trees. Identity success still
does not claim hardware acceptance.

PC validation is source/text only. COPR, signed DNF, identity on the real
tablet, USB enumeration and first-setup commissioning remain separate gates.
