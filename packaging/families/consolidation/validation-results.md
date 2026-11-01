# Validation results

1 November 2026: the additional [original Fedora comparison](fedora-return-audit.md)
passed its signed static checks for 13 libraries, 219 interface files and 836
libwacom database files. Original-package migration remains unadmitted; this
does not reduce the active producer or installed binary count.

1 November 2026. Source editing and RPM inspection on Fedora Rawhide 46
AArch64; all native compilation and package checks run in COPR. No tablet
installation, hardware test, boot write or reboot was performed.

| Scope | Evidence | Result |
|---|---|---|
| Source | GitHub main `50e18c0`, source-contract run `41274722360` | Passed with native compilers and rpmbuild refused |
| Source | Bash syntax, split-spec fixtures, whitespace | Passed, including independent child Version/Epoch, monotonic rebuild Releases, exact ISA dependency and unchanged legacy compatibility bounds |
| Inventory | Official COPR API, 36 original entries and empty offset 36 page | Captured in inventory.json; source pins differ from main |
| Source archive | 14 complete baseline SRPMs, hash-checked | Preserved in the published rollback archive; archive assets verified against GitHub SHA256 digests |
| Native meta | Build 11239542, 1.0.0-2 | Passed; five original binary identities, replacing the earlier 11239474 |
| Native audio | Build 11239483, 1.0.0-1 | Passed; three original binary identities |
| Native sensors | Build 11239521, 1.0.0-3 | Passed; three runtime and two optional debug identities |
| Native power | Build 11239485, 1.0.0-1 | Passed; four original binary identities |
| Native video | Build 11239487, 1.0.0-1 | Passed; two original binary identities |
| Native extras | Build 11239488, 1.0.0-1 | Passed; two original binary identities |
| Signed RPM | 21 baseline and 21 new binary signatures; key `2204A661EA630A5303500085538797E72A3E4ECA` in an isolated RPM keyring | Passed |
| Runtime metadata | Original names/versions, increased Releases, dependencies, compatibility bounds, scriptlets, file paths/modes/owners/config flags and non-ELF data hashes | Equivalent after only self-version/config capability normalization and generated ELF build-id path exclusion |
| Debug data | Original two optional package names, versions, licenses, no new scriptlets, paths limited to debug/source/doc/license trees | Passed; source/build-id layout changes with the grouped source, not byte-identical debug payloads |
| Solver | COPR source worker 11239542; real Fedora/Uke metadata including filelists and signed baseline/candidate RPMs | 15 plans passed: fresh, upgrade, mixed, reverse and weak-deps-off across core, KDE and all admitted runtime targets; no dependency transaction executed |
| Config migration | COPR source worker 11239542; RPM transactions relocated into fresh temporary directories, scriptlets/sysusers/triggers disabled | Modified configurations in adaptive-display (3), charge-control (2) and speaker-orientation (1) survive upgrade and rollback; removal retains `.rpmsave` |
| Service migration | Existing source-family service-state fixture in 11239542 | Enabled/active preservation, disabled/masked refusal and restoration-failure checks passed with stub service tools; no device service invoked |
| Device | Installation, boot, touch, audio, suspend, sensors and charging | Not performed |

Sensor build 11239484 passed its original checks but omitted optional debug
packages. It is superseded by 11239521. Build 11239511 correctly failed the
unpackaged-file check after debug extraction; explicit debug package
declarations corrected the source. Validation job 11239524 failed before SRPM
generation because its source worker lacked `rg`; the fixture now uses the
worker's existing grep. Later source-only trials exposed missing filelists,
the need for a pinned key in each isolated RPM database and RPM 6's separate
sysusers hook. All were corrected before 11239542. These failures are not
reported as successful gates.

[The archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101)
retains 14 SRPMs, 21 original signed binaries, the original public key and
SHA256SUMS. No original COPR definition or build was deleted. The live project
has 42 definitions: 28 active sources and 14 retained definitions whose source
scripts explicitly refuse new standalone builds. Six integration scripts use
the pinned reviewed generator and refuse regeneration of a known source
revision. The independent routing audit verifies only those 20 script fields
changed; the other 22 definitions, source settings and project settings match.
See routing-receipt.json and binary-audit.json.

Cutefish metadata is preserved and signed, but its separately packaged desktop
provider is outside the Fedora/Uke repository fixture. It is explicitly excluded
from admitted solver selections, not counted as a passing desktop installation.
GNOME has no separate selection RPM in this source snapshot. The core graph
still includes the existing adaptive-display KDE dependencies: fresh core
resolves 671 incoming packages normally and 485 with weak dependencies off.
This source consolidation changes neither that dependency policy nor the fork
count. Conditional Fedora replacements remain subject to the report's policy,
upstream-fix and ABI gates; their removal is not claimed.
