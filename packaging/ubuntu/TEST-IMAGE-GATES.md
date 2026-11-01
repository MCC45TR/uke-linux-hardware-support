# Minimal Tenacious ARM64 test image gates

The user now authorizes preparing an Ubuntu test image for later download.
No tablet write, partition operation or boot-slot selection is authorized here.
Fedora DE release work stays independent and retains its existing cohort.

## Current concrete preparation

- Experimental kernel 1006 is pinned to commit
  `fcbdae4b757732f0f0e3162c610f19abae290832` and COPR source build 11240463.
- Native Debian source version `7.3.1-1006ubuntu1~tenacious1` is assembled.
  Original Linux signature, all emitted profile/boot inventories, chunk
  reconstruction and source-package checksums pass without source execution.
- Candidate identity `7.3.1-1006u1` satisfies the shared ALSA bound; its recipe,
  inspector and refusal fixtures agree. Firmware, boot and OEM publication
  gates still fail closed.
- The first PPA speaker preset is Published for Tenacious ARM64. Its existing
  archive key is `27A0465D56AF763E969F432D6A0B0B9CF4A769FB`, distinct from the
  user's OpenPGP key. No private key is needed for the recipe pipeline.
- `Ubuntu ARM64 test userspace qualification` bootstraps signed official Ubuntu
  packages and verifies that archive key before installing the exact qualified
  PPA package in a disposable server container. It checks Python exclusion,
  systemd, SELinux policy presence and the preset hash. Its artifacts contain
  evidence only; no root image, kernel, initramfs or boot bundle is emitted.
- [Userspace retry 41319778138](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41319778138)
  passed. The [host boot adapter](BOOT-ADAPTER.md) now stages and passes metadata
  refusal fixtures, but its native dracut/root-handoff execution remains untested.

## Required work before emitting a test image

1. Obtain source-specific approval for native server execution of the prepared
   1006 set, including shared preparer/sanitizer fixtures, boot helpers and the
   normal disposable kernel module-signing key. No tablet execution is involved.
2. Pass native kernel compilation and real DT/module inspection. The successful
   Fedora test-channel build is not an Ubuntu build or boot result.
3. Natively qualify the prepared systemd/dracut dpkg provenance and release
   adapter without weakening filesystem/root-selector, timeout, recovery,
   privacy or target-payload tests. Keep the base free of firmware, host
   identity, Python and stale root-specific command lines.
4. Validate a complete same-job Image/DT/modules/embedded-initramfs cohort,
   preserving module-key identity through both Image passes. Candidate-only
   Image packaging is not a substitute for this step.
5. Validate the selected existing Uke root layout and explicitly resolve the
   security profile. Ubuntu stock uses AppArmor, which the current shared
   configuration does not support. Retaining SELinux instead needs policy,
   labeling and enforcing handoff qualification; adding AppArmor support needs
   reviewed configuration and authorization. The adapter makes neither choice
   and changes no security setting. Do not disable checks to obtain a boot.
6. Prove the offline image manifests, root-selector capability, module and
   source identities before exporting a test bundle. Firmware redistribution
   review remains incomplete; do not include unqualified OEM/private payloads.
   Missing runtime ABI providers must stay excluded rather than fabricated.

Only after these gates pass may the image writer produce the actual root and
boot artifacts. Physical boot/display/network acceptance is a subsequent user
test; no successful hardware boot is asserted by host checks.
