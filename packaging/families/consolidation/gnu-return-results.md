# Original Fedora GNU runtime return

1 November 2026. Fedora Rawhide 46 AArch64. The original Fedora GNU C++ runtime
replaces the fourth policy-only fork. The current COPR listing has 43 source
definitions: 25 active producers and 18 retained definitions whose scripts
refuse another build. The original 36-source scope now has 24 active producers;
the independent color-profile source accounts for the additional live producer.
No definition or historical build was deleted.

| Qualified source | Build | Result |
|---|---:|---|
| Boot integration | [11239841](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239841/) | 1.8.18-2.fc46; six existing binary identities |
| Power integration | [11239842](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239842/) | 1.0.0-2; three battery/helper packages at 1.4.1-3 and adaptive-display at 1.0.16-14 |
| Meta integration | [11239896](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239896/) | 1.0.0-8; Core 1.0.7-16 and four desktop children at 1.0.0-32 |
| Retained Control manager | [11239677](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239677/) | 2.8.0-2.fc46; the separately gated GNU mapping is unchanged |

The [receipt](gnu-return-results.json) records source commits, signed binary
hashes and all 13 original-Fedora mappings. GNU ABI and signatures were checked
against original `libstdc++-16.2.1-3.fc46.aarch64`. The approved Fedora-root
policy retains distribution-provided helpers and dependencies, including GDB
Python files in console profiles. Project code uses C++ or shell. Recovery,
initramfs and installers remain Python-free.

Eight explicit Core, boot and power Requires now name the real Fedora
`libstdc++(aarch-64) >= 16.2.1` provider. Core requires compatible boot and
battery versions and excludes older optional charge callers. The existing
`uke-core-meta(fedora-cxx-runtime-v1)` gate admits the GNU mapping; no alias
claims the former custom runtime capability. The complete qualified boot
1.8.18 source archive is retained with unchanged non-spec program sources.
All ten boot/power RPMs preserve scriptlets, ownership, configuration flags
and non-ELF bytes. The five final meta binaries match the tested candidate's
payload, Requires, scriptlets and ownership at higher Releases.

Ten source-worker admission cases and
[18 signed package plans](gnu-return-solver-results.txt) passed. Core, KDE and
all admitted meta selections cover fresh runtime, skipped/mixed upgrades,
reverse plans and disabled weak dependencies. Three additional cases start
from the immediately preceding desktop-return meta cohort: all 12 Fedora
names remain original and exactly the GNU fork is replaced. Reverse plans
use the earlier full-fork baseline and retain Control 2.8. Each disposable
root retains the exact five-part kernel `7.3.1-106.fc46` tuple; no kernel
enters or leaves the plan. These are solver roots, not a blank-root installer.
Cutefish remains outside the admitted provider set.

A temporary COPR dracut dependency archive includes the native boot guard and
original Fedora C++ library with zero Python files. It includes no kernel and
was not booted. A complete tablet initramfs and physical acceptance remain
separate image/device gates. No RPM transaction, tablet installation, reboot,
boot write, charging operation or firmware access was performed by this change.

Prepare the compatible Control 2.8 manager before a device migration. Review
`sudo uke-control brunch unstable --dry-run --json` against the authorized
matched device cohort before applying a transaction. The manager admits
original Fedora repositories, vendor, source owner, architecture, release and
version floor, and removes only the reviewed old NEVRAs. Unknown fork versions
and unresolved legacy consumers refuse the transaction.

The [rollback archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101)
contains 82 assets. [GNU-RETURN-SHA256SUMS](GNU-RETURN-SHA256SUMS) covers 23
current rollback/source inputs: four complete SRPMs, sixteen old runtime
binaries and three retained manager binaries. All digests match GitHub; the
61 earlier assets and checksum files are unchanged. Four source script fields
and the meta worker's development dependencies changed in this phase. Project
settings, automatic rebuild flags and unrelated definitions were preserved.
The independent kernel recipe remains separate from its main branch.

The active native-source catalog now contains five functional forks. The four
returned sources retain complete provenance in its withdrawn list and cannot
be selected by its source-generation allowlist or Rawhide tracker. The
libaccounts-glib destructor correction, firmware licensing boundary, separate
recovery source and physical kernel/DT gates remain as documented in the plan.
