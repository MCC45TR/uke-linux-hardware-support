# UKE-UBUNTU-20261101: separate source packaging from device admission

Date: 1 November 2026. Environment: Debian 13 x86_64; dpkg-source 1.22.22.

Evidence: the public mcc45tr/uke-linux-unstable COPR listing contains 43 source
entries, including superseded standalone recipes. Common source commit
cbecc441059ebffa97497e736edd2e0b0b0546a5 has no existing Debian packaging.
The Control RPM explicitly requires libdnf5, RPM, dracut and SELinux integration;
translating dependency names cannot port its transaction implementation.

Resolution: add Debian overlays without changing shared C++ or Fedora specs.
Unsigned battery-observer and front-color source packages assemble/extract with
checksums and no install-time systemd activation. The latter still depends on
an unported SSC service. All other source routes retain explicit coverage states.
Tests: packaging/ubuntu/tests/source.sh passes on the stated host; no upstream
binary was built or executed, and no physical acceptance is inferred.

Uncertainty: target dependency resolution, ARM64 build/install behavior, exact
kernel/DTB/initramfs cohort admission and current firmware redistribution rights.
Next validation: approved Tenacious ARM64 builds and service lifecycle tests;
port APT/dpkg transaction integration and boot cohorts before publication.
PPA terms, source signing and upload authorization remain parent-owned blockers.
