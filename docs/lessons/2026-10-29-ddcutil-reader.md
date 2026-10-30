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

## UKE-DDC-PKG-L002: Verify the native regression and signed binary payloads

- **Date:** 2026-10-29.
- **Environment scope:** COPR Fedora Rawhide AArch64; package inspection on
  POCO Pad X1 without installation or execution of the built CLI/library.
- **Evidence class:** Build and package, separate from monitor hardware.
- **Status:** Native build and binary audit pass; device acceptance remains open.
- **Finding:** [Build 11228224](https://copr.fedorainfracloud.org/coprs/build/11228224)
  produces `ddcutil`, `libddcutil`, development and debug subpackages at
  `3.0.2-2.uke1.fc46` from source commit
  `4335808aa1bafe9c2adf23326db621f75539f599`. The sanitizer fixture passes for
  the patched production function; reversing the patch makes the original
  function fail its regression assertion with exit 134.
- **Evidence:** All six AArch64 binaries pass header/payload digest and RSA
  signature checks in an isolated RPM database containing only project key
  `2204A661EA630A5303500085538797E72A3E4ECA`. The source-worker SRPM is
  unsigned; its digests pass and SHA-256 is
  `4e1e5c456d426cf03db59e2a7bbd0a3078a166383e9b82704a118a6e53d79405`.
  Its patch and test match the source commit. Native debug `file_util.c`
  compares exactly with the patched pinned source and hashes to
  `07aa1759e564da05956ec159e234fc502a4f81d62fa7617ac788133b64d65ec1`.
  The CLI and shared library are AArch64 ELF objects; RPM-generated provides
  and development symlinks retain `libddcutil.so.5`. No scriptlet, Python path,
  regression helper or sanitizer runtime dependency appears in the runtime
  packages. The installed direct runtime providers satisfy the required ABI;
  i2c-tools is 4.4. A full fresh repository dependency solve is not performed.
- **Practical consequence:** The separate upstream-named source entry is
  available to the coordinated unstable DNF migration. No aggregate family
  package, additional service, global key import or device operation is added.
- **Remaining uncertainty:** Reader correctness does not establish valid Juo
  DDC replies, monitor brightness control or DSC transport acceptance. Target
  installation, transitive dependency closure and physical tests remain separate.
- **Next validation:** Complete the coordinated signed DNF transaction, then
  verify the installed reader and valid reversible monitor control in an
  operator-approved display window.
- **Supersedes / superseded by:** Closes only the pending native build and
  inspected binary-payload gates in UKE-DDC-PKG-L001.

## UKE-DDC-PKG-L003: Keep installed reader behavior separate from DDC forwarding

On 30 October 2026, installed `ddcutil`/`libddcutil` `3.0.2-2.uke1.fc46`
pass RPM verification and match the audited native CLI/library payloads exactly.
On POCO Pad X1 running Linux `7.3.0-49`, root brightness GetVCP fails without
the preceding empty-reader warnings. A locked raw request again receives eleven
zero bytes despite successful I2C command status. A fixed-gray Wayland pattern
and tablet-connected Konftel Cam10 show no visible brightness change for
accepted 20/70 commands and the historical return request 45. Because GetVCP
fails, 45 is not described as a verified original setting or restoration.

After separately approved activation and reboot into installed Linux
`7.3.0-54`, explicit root GetVCP still returns `VCP 10 ERR`, exit one. Camera
USB disconnects prevent a fresh sustained optical comparison in that boot.
No additional SetVCP is issued there. This closes the installed-package
checkpoint but leaves DDC forwarding and monitor-control acceptance open.
Raw media, timing, measurement limits and the failed native camera capture
remain in the canonical private Juo lesson journal, UKE-DDC-L016-L017 and
UKE-DP-REC-L029. No new source fix, local compilation or hardware success is
inferred from these negative trials.
