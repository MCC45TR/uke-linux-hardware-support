# Control source family

1 November 2026. Fedora Rawhide 46, AArch64. The existing `uke-control`
source now owns the 12 independently installable RPMs below. It replaces the
three Control, meta-selection and native power/display source producers.
No new COPR definition or runtime umbrella RPM is added.

| Existing binary packages | Previous Release | Family Release |
|---|---:|---:|
| uke-control, uke-control-tests, uke-control-tui 2.8.0 | 2 | 4 |
| uke-core-meta 1.0.7 | 16 | 18 |
| uke-desktop-metas, uke-kde-plasma-meta, uke-cutefish-meta 1.0.0 | 32 | 34 |
| uke-plasma-controls 1.0.0, noarch | 32 | 34 |
| uke-battery-observer, uke-charge-control, uke-offline-charge 1.4.1 | 3 | 5 |
| uke-adaptive-display 1.0.16 | 14 | 16 |

All Releases use the Fedora 46 distribution suffix. The source package is
`uke-control-2.8.0-4.fc46`. The profile's `release: 2` is the increment above
each pinned input; the retained primary source starts at Release 2.

Complete inputs are builds 11239677, 11239896 and 11239842. Their three SRPMs
and 12 signed baseline binaries are preserved in the
[permanent source and rollback archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101).
[CONTROL-FAMILY-SHA256SUMS](CONTROL-FAMILY-SHA256SUMS) records every digest.
The manifest uses durable archive URLs and retains each original COPR URL and
build identity. The 13 additions leave the earlier 82 archive assets unchanged.

The composer retains the primary Control identity and independent child
Versions/Releases. It omits the two source-only parent names, retains all
owning build checks and licenses, and increases every child's Release. Native
program sources, dependencies, optional selections, service scripts and disabled
hardware defaults retain their existing contracts. No original KDE or
Fedora package is rebuilt as part of this family.

[Build 11240074](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11240074/)
passed the first full native build with 12 signed RPMs. The source worker for
[build 11240089](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11240089/)
then checked those exact signed candidate packages against the complete current
baseline before generating the fresh Release-4 source. The generator pin is
`7949a25c1032161612f3e857524fa5e67f9b61ae`.
Build 11240089 subsequently passed the full native package checks.

The 20 real libdnf5 plans cover fresh, upgrade, mixed, rollback and weak-deps-off
for Core, KDE, all available runtime children and Control alone. Every disposable
root retains the five signed kernel-106 RPMs; a kernel change is an error.
No dependency transaction or target scriptlet is executed. Cutefish metadata
is retained, but its external desktop provider is outside this fixture.
See [the plan and config results](control-family-solver-results.txt).

Real RPM operations relocated into temporary directories preserve three
Adaptive Display and two Charge Control modified configuration files through
upgrade and rollback, then retain `.rpmsave` on removal. Scriptlets, triggers
and the separate RPM 6 sysusers hook are disabled for this fixture.

Signed inspection compares all 12 names, Epochs, Versions, architectures,
licenses, dependencies, compatibility capabilities, scriptlets, triggers,
file paths/modes/owners/config flags and non-ELF hashes. Only cohort EVRs and
generated ELF build-id paths are normalized; compiled ELF bytes are not claimed
identical. Both Control libraries retain their SONAMEs and their exact exported
symbol names: 111 client symbols and 211 backend symbols. The
[binary receipt](control-family-binary-audit.json) records the final package hashes.

Routing retains 43 visible source definitions: 23 active producers and 20
historical definitions that refuse new builds. The two retired integration
recipes point to the Control family. Only these three recipes and Control's
source-worker dependency union change; unrelated source and project settings
are preserved. The [routing receipt](control-family-results.json) records this
boundary. No source definition or historical build is deleted.

Original Fedora dependencies and helpers remain allowed on the Fedora root.
This change adds no project Python code or runtime, and does not authorize
Python in recovery, initramfs or installers. It does not qualify a full image,
installed session, physical charging/display behavior, boot or device rollback.
