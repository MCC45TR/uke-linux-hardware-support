# Preserve boot profiles across consolidated kernel updates

- ID: UKE-SUPER-BOOT-CONSUMER-20261116
- Environment: Fedora Rawhide AArch64, consolidated Control23 consumer and the
  separately qualified seed4 kernel/codec sources; source inspection on x86_64.
- Evidence: the actual signed Control23 source embeds boot integration 1.8.102,
  then applies health, critical rollback, session and closed-cover overlays.
  Its strict header allowlist rejects new super flags, and wrapper derivation
  starts from the canonical package image. Editing the older standalone boot
  checkout would not update the installed consumer.
- Consequence: Control24 targets the exact three prepared source files with
  pinned pre/post hashes and a whole-file reverse comparison. It retains all77
  old source inputs and the complete parent recipe through an inverse. Existing
  Control ownership carries the change; no new runtime package is necessary.
- Slot identity: the measured boot handoff has no Android slot suffix. The
  existing unique-running-Image fallback becomes ambiguous when both slots hold
  the same kernel. Capable images therefore receive their physical destination's
  standard Android suffix. The native kernel parser already admits this field.
  A misplaced suffix is rejected rather than converted into a GPT-based guess.
- Recovery: forward super updates require the exact aligned capability note.
  Existing legacy kernels retain their wrappers; selecting stored recovery
  derives that image's own profile. No protection or ownership gate is removed.
- Validation: source-only overlay application and complete inverse/postimage
  comparison passed. Four compiler/sanitizer modes against actual updater
  functions and the complete production CLI are required in COPR. Native and
  physical results remain pending at this checkpoint.
- Limit: this is updater compatibility, not proof of rescue boot, logical swap,
  encryption, hibernation, hardware sealing or 256 GB firmware handoff. OEM slot
  injection differs between bootloaders; duplicate or invalid suffixes remain
  refused until matching physical evidence exists.
- Next: inspect the actual COPR source and native results, deliver signed Control
  through DNF, then qualify the annotated next kernel before full ZIP testing.
