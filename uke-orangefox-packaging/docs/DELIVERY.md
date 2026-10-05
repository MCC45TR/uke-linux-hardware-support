# DNF-managed recovery image delivery

The initial package checkpoint delivers the exact GitHub release
`r12.0-uke.20260930-alpha1`, preserving its **alpha** classification and
**Global OS3.0.303.0.WOZMIXM** firmware scope. The release has compile/static
validation but no physical boot or rollback acceptance. Later source features
are not implicitly included in this historical alpha.

```sh
sudo dnf copr enable mcc45tr/uke-linux-test
sudo dnf install uke-orangefox-recovery
sudo dnf upgrade uke-orangefox-recovery
```

Files are data under `/usr/share/senemos/recovery/uke/VERSION/`.
The RPM has no scriptlets, service, kernel-install hook or automatic installer.
DNF updates the delivered images; it does not write Android partitions, unlock
the bootloader, select a boot entry or execute the ZIP's installer.

Inspect `ARTIFACT-MANIFEST.json`, `release-lock.json`, `INSTALL.md` and
`PACKAGE-SHA256SUMS` before an independently authorized device evaluation.
Do not apply this profile to a different installed firmware. RPM/package tests
and QEMU userspace tests do not establish recovery boot or hardware support.

There is no published stable recovery release at this checkpoint. Stable polling
selects only non-draft, non-prerelease releases with a valid public validation
manifest. Alpha delivery is an explicit test-channel pin and cannot advance
the stable tracker silently.

The authoritative version, channel, firmware scope and source offers of each
installed package are in its accompanying `release-lock.json`. A future verified
stable release replaces the pin through the scheduled workflow. The historical
alpha checkpoint above does not classify future stable artifacts.
