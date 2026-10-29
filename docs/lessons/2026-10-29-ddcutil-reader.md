# Native packaging of the ddcutil first-line correction

- **Lesson ID:** UKE-DDC-PKG-L001
- **Date:** 2026-10-29
- **Environment scope:** Fedora Rawhide AArch64, POCO Pad X1; source inspection.
- **Evidence class:** Source; native COPR build and package results pending.
- **Status:** Prepared, without device installation or monitor access.
- **Finding:** The official Fedora `ddcutil-3.0.2-1.fc46.src.rpm` contains the
  same first-line reader as upstream commit
  `5085ae9bb1a8e5bd8db717d80dd80a279a0f05fb`; their source files compare exactly.
  The retained patch returns null after a failed read and preserves an
  unterminated last character. Native regression checks exercise the extracted
  production function with sanitizers, rather than a reimplementation.
- **Evidence:** The exact Koji source RPM has SHA-256
  `666546358c31cad3f0be38e07ec894c4bb91f1c889242342efb4d6c453440152`.
  The official signed Rawhide mirror has SHA-256
  `94fae9998f205ea9d1993cfce2472a6f4215df74d7ad059d2110ba715115d43c`;
  its signature passes an isolated RPM database containing only the Fedora 46
  key `D924B10D3E810DABDD8B56B596E7E91491211FCE`. Its complete decompressed CPIO
  payload compares exactly with the immutable Koji source and hashes to
  `ced766244babcfcfac8055f812a066817c6aa0cd355419561f98da398e28ee0a`.
  Source, patch and test are under `uke-desktop-packaging/packaging/ddcutil`
  and `uke-desktop-packaging/tests/ddcutil-first-line.sh`.
- **Practical consequence:** Prepare a separate `ddcutil` source entry in the
  Rawhide unstable project, with ordinary CLI/library development subpackages.
  Compile and run regression checks only in COPR. Preserve Fedora licensing,
  configuration and ABI; no additional service or hardware probe is introduced.
- **Remaining uncertainty:** This is not a correction of the invalid raw Juo
  DDC/CI replies. Source checks cannot establish native package or monitor
  acceptance. The active tablet remains outside this source-build operation.
- **Next validation:** Native COPR checks, signed RPM/ELF/dependency/payload
  audit, followed by a separately coordinated install and monitor test.
