# Independent Rawhide source packages and pending channel enrollment

- **Lesson ID:** UKE-COPR-UNSTABLE-L001
- **Date:** 2026-10-29
- **Environment scope:** Fedora Rawhide AArch64 on POCO Pad X1; attached USB SSD.
- **Evidence class:** Source and existing-system inspection.
- **Status:** Draft; native build, publication and installed migration pending.
- **Question or previous assumption:** Whether changing the COPR project name
  alone can replace the nine aggregate source builds with independent packages.
- **Finding:** Installed selection RPMs and Core metadata still require the
  legacy family identities. Thirty independent source recipes therefore retain
  reviewed child recipes and exact intra-component requirements, retire empty
  selection RPMs with explicit Provides/Obsoletes, and retain installation
  dependencies in actual Core metadata. Native libraries remain independent.
- **Evidence:** Source baseline `6fd84c6331e1a9d7aaaadd67db21b7f0b933b036`;
  `packaging/standalone/packages.json`, `host/emit.sh` and the shared composer.
  Local manifest, existing AWK recipe/file-list fixtures, Bash syntax and
  whitespace checks pass. The source-RPM fixture and native C++ tests are
  assigned to COPR and have not run for this draft.
- **Practical consequence:** Register only independent component source entries
  in `mcc45tr/uke-linux-unstable`, with exactly `fedora-rawhide-aarch64` and Fedora
  branching disabled. Keep matched kernel children in one kernel source build.
- **Remaining uncertainty:** The current COPR API credential is rejected, so the
  project has not been created. Its public signing fingerprint cannot yet be
  enrolled; the draft manager refuses imports. Device channel selection and
  installed RPMs are unchanged. No local native build or reboot was performed.
- **Next validation:** Renew API access, create the project, enroll its public
  key, then run COPR source/native checks, signed package audits and the actual
  manager preview before installation and channel migration. Retain the existing
  kernel activation gate and request operator approval before rebooting.

The installed system identifies Rawhide as `46 (Rawhide Prerelease)` with
`RELEASE_TYPE=development`. The former source parser accepted only the literal
`Rawhide` name or VERSION_ID. The draft parser and source-worker admission also
accept the observed structured form, while release/Beta rejection fixtures stay
in place. This is a source correction, not an installed manager test.
