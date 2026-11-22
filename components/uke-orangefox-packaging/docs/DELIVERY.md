# DNF-managed recovery delivery

Updated 14 October 2026 for Fedora Rawhide AArch64. The package delivers the
existing `R12.0-uke-modded-v003` preview, including its exact IMG/ZIP bytes,
source pins, checksums and license records. This is a packaging update; the
recovery was not rebuilt. The withdrawn Alpha 1 release is no longer an input.

```sh
sudo dnf copr enable mcc45tr/uke-linux-test
sudo dnf install uke-orangefox-recovery
sudo dnf upgrade uke-orangefox-recovery
```

Files are data under `/usr/share/senemos/recovery/uke/VERSION/`. The RPM has no
scriptlets, service or automatic installer. DNF does not write Android
partitions, unlock the bootloader or change boot selection.

The IMG contains a ramdisk and requires the installed Android boot stack.
It is not a complete temporary `fastboot boot` image. The ZIP installer is
unavailable. Compatibility with the installed firmware, cold boot, charging,
storage writes and rollback require separate device validation.

Read `ARTIFACT-MANIFEST.json`, `release-lock.json`, `INSTALL.md`, the source
offer and `PACKAGE-SHA256SUMS` before testing. The OS3.0.303.0 source profile
does not establish compatibility with other firmware or installed modules.

Preview delivery is explicitly pinned. Stable tracking accepts only eligible
non-prerelease releases with the complete reviewed artifact/source contract;
it does not silently promote this preview or skip failed checks.
