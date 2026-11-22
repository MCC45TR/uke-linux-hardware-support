# GPT maintenance with rollback disabled

On POCO Pad X1, the first boot of kernel7.3.1-12209-unstable in slotB
leaves two identical partition arrays and a valid primary header/array, but a
valid backup header contains an outdated array CRC. Both independent native
readers confirm this state. No divergent array or damaged header is admitted.

Updater100 already recognizes this exact condition after boot and validates
early firmware, running Image, protected trial identity, UFS selection and
all paired slot flags. Its rollback-off acceptance branch returns before the
existing maintenance commit. Consequently read-only status correctly refuses
the damaged backup header, and later automatic staging cannot proceed safely.

The update repairs this admitted CRC condition before the rollback-off return.
It uses the existing bounded GPT serializer, metadata journal, readback and
restoration path. A maintenance helper rejects pending flag/type changes.
Preview creates no journal and writes nothing. Kernel health approval,
checkpoint state, slot attributes, GUIDs, UFS selection and boot images retain
their existing meaning. Package defaults remain rollback-enforced; a selected
local preference remains unchanged across upgrades.

The complete100 source capsule and original package checks are retained.
Actual regular-file GPT tests cover512/4096 sectors, both selected slots,
preview, commit, repeat calls, unchanged approval bytes and rejection of a
pending success/selection mutation. The existing GCC/Clang and sanitizer
matrix executes these cases. Both compilers reject a skipped-commit mutant.
Passing these tests is separate from target DNF and physical boot acceptance.

The independently inspected Qualcomm ABL reference is
CodeLinaro-mirror/la_abl_tianocore_edk2 at
aef4e86045bd151a54d33bab8ae5e8e62462a8f4,
QcomModulePkg/Library/BootLib/PartitionTableUpdate.c,
Git blob6cd565fb52199c17ffb7bfed72c98865e580041b, BSD-3-Clause.
Its partition update code recomputes table and header checksums. That source
is not proof of the fitted OEM binary or the exact origin of the observed
stale field. It remains unchanged reference evidence; no firmware is written.
