# Supplier observer packaging

3 November 2026. `host/copr-pack-variants.sh` carries the reviewed NVT/Sunwoda
supplier hint into `uke-battery-observer 1.4.1-7.fc46` and its two existing
charge/offline children. The current repository's native source had not reached
the retained Control family's battery archive. Published battery release6 is
therefore the comparison baseline, despite newer Control frontends.

The source worker verifies Control COPR11249690 and all three nested source
inventories. It replaces only the observer collector, bounded classifier,
README, Makefile and supplier tests. Every other source file is checked against
the retained byte inventory. The original Fedora C++ runtime dependencies,
config files, units, maintainer scripts, charge controls and qualification
defaults are retained. No persistent COPR source definition is added or changed.

The standard `manufacturer` and `model_name` fields supply a bounded text hint.
Unknown and conflicting identities remain explicit. A supplier name does not
identify gauge silicon, qualify a battery, enable an electrical profile or
prove a charging limit. No firmware blob or unit calibration is included.

The original native `%check` exercises classifier, collector, model, D-Bus,
charge-control, offline display and admission fixtures in the COPR worker.
Physical battery variants, cutoff, charging, thermal response and suspend
remain untested by this build. Package installation/restart/reboot is separate
from source generation. Retain the prior three matching release6 packages for
a controlled rollback; package build success does not establish that rollback.

COPR [11250670](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11250670/)
completed on 3 November 2026 from source
`f19db5a87e707271323a9e8fabf605e0aae60f0d`. All three AArch64 RPMs are
`1.4.1-7.fc46`; software checks and downloaded RPM digest/signature checks pass.
The initial source-only submission11250635 stopped at a dependency-order
assertion before producing an SRPM. The corrected guard retains all three
Fedora runtime dependencies, including the offline helper's leading systemd
dependency. No tablet installation or physical qualification was performed.
