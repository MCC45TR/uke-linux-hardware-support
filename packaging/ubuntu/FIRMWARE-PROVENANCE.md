# Firmware and OEM resource publication boundary

Recorded 1 November 2026. Publication decisions apply to exact files, not to a
device, source archive or package family as a whole.

The shared `xiaomi-uke-firmware/manifests/firmware-notice-review-20261030.json`
records 217 firmware/configuration files. Only seven have exact notice mappings;
74 original notice paths are missing and 136 named paths remain unmatched.
Its `redistribution_review_complete` and `firmware_blob_package_ready` fields
are false. `prepare-source.sh uke-firmware` therefore fails explicitly before
creating an output directory. No incomplete firmware bundle is staged here.

The **separate speaker preset** has its own exact Apache-2.0 attribution. The
shared archive's 4,828-byte `/odm/firmware/fs19xx.fsm` has SHA-256
`f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b`.
`verify-speaker-provenance.sh` checks that hash, original compressed ODM notice,
exact filename/content-ID mapping and license text. The Debian recipe retains
these records and declares the preset a conffile, preserving local changes.
It ships no unit calibration, persist data, program or activation hook. This
file-specific attribution does not admit the other firmware files or establish
speaker playback/protection qualification.

The OEM-resource helper at shared commit
`53ae6371b12758e01fe0af4690e6fb54d2e1b436` is MIT code, with no OEM blob in its
source package. Its runtime reads protected OEM partitions through read-only
loop mappings, rejects nonlinear/snapshot metadata, bounds DTBO extraction,
checks ownership during cleanup and only selects byte-identical SSC defaults.
It also unconditionally requests a SELinux mount context. Ubuntu portability
must preserve those guards and handle the actual security module; silently
dropping mount restrictions or starting it automatically is not an acceptable
port. `uke-oem-resources` source preparation is currently blocked with that
specific implementation reason. No partition or helper was accessed/executed.

SSC release 26's optional OEM-reference path is already preserved. It falls
back to packaged inputs when references are unavailable; this does not supply
the missing firmware dependency or justify a fabricated ABI provider.
