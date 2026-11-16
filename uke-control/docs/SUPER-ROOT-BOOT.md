# Seed4 boot profile updates

Control24 extends the existing consolidated kernel updater. It preserves the
explicit normal `senemos.super=seed4-v1` profile during forward updates. The
kernel must contain the exact unique aligned capability note; a command-line
string alone is insufficient. Existing kernel verification, package signatures,
module closure, ownership, power, firmware and transactional rollback gates stay
in place. No additional runtime RPM, service or polling loop is introduced.

New capable kernels receive a destination-specific Android slot suffix. Both
boot partitions can then contain the same kernel Image while the running slot
remains verifiable. An annotation stored in the wrong physical slot is refused;
the selected GPT slot alone never proves the running slot. Existing kernels keep
their original wrappers. Selecting retained recovery checks that image's own
profile and does not force the new flag onto an older kernel.

The root-only `derive-slot-image INPUT OUTPUT a|b` command assists offline image
assembly. It accepts protected regular files, exclusively creates its output,
preserves the kernel Image and verifies full readback. It performs no device,
GPT, firmware, key or resume operation. The existing codec creates the rescue
profile separately; this updater does not turn a rescue boot into a writable
userdata update.

Native checks exercise actual updater functions with synthetic boot images in
four GCC/Clang and sanitizer combinations, including legacy recovery, duplicate
or misplaced slot declarations, corrupt/missing/duplicate capability notes and
malformed profiles. Production CLI compilation retains all warning checks.
These checks do not qualify physical rescue boot, capacity variants, encryption,
hibernate or hardware key release. Those require separate device validation.
