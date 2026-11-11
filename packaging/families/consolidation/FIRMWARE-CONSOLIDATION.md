# Firmware package ownership

11 November 2026: firmware3.0.304.0-15 folds the generic FS19xx preset from
speaker-firmware3.0.304.0-2 into the existing firmware package. Both actual
source archives remain unchanged. The exact preset SHA256, original notice,
Apache license, provenance and documentation are retained. Its existing
`config(noreplace)` flag preserves local modifications. No executable, service,
device calibration, firmware operation or new runtime RPM is added.

The complete firmware14 recipe is inverted by the preparation check. Original
217-file content/hash/byte comparisons run before the additional preset is
installed and checked; final218-file coverage supplements those gates.
Native packaging, signed payload/flags parity, DNF upgrade/fresh/rollback and
device ownership must pass before retiring the speaker firmware producer.
Keep exact signed firmware14 and speaker2 recovery outside expired COPR storage.
Package ownership does not qualify physical audio or protection effectiveness.
