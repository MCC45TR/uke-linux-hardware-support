# Project-owned recovery planning source

Imported without edits from recovery-uke-ofox commit
`8395560393f78bcbe2adeaedc44057c0aa7141de`,
`src/device/xiaomi/uke/recoveryctl/libuke/`.
Each source file retains its Apache-2.0 SPDX notice. See LICENSE-APACHE-2.0.

Control reuses the GPT reader, original-userdata provenance checks, bounded
allocation policy and GPT proposal generator. The Fedora executor is separate.
Existing recovery live-write admission is preserved; its generic writers are
not linked into the layout helper. The helper uses only its own filtered
userdata/GPT commit path after checking all protected OEM record bytes.

The source receipt contains per-file hashes. Reference/recovery build scripts
are not executed or shipped. Regular-image tests are not tablet acceptance.
