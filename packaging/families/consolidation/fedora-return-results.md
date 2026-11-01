# Original Fedora desktop package returns

1 November 2026. Fedora Rawhide 46 AArch64. Control `2.8.0-2.fc46`
([11239677](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239677/))
and meta integration `1.0.0-4`
([11239731](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11239731/))
passed native COPR checks. The unstable source recipes now use those qualified
inputs. Three desktop forks stop producing new builds; their existing signed
packages, source offers and build history remain available.

| Retained source definition | Original Fedora source | Mapped runtime/development RPMs |
|---|---|---:|
| uke-at-spi2-core | at-spi2-core | 6 |
| uke-gstreamer1 | gstreamer1 | 2 |
| uke-libwacom | libwacom | 4 |

The [receipt](fedora-return-results.json) records exact source commits,
signatures, binary hashes and mappings. The static comparison covers 13
original/fork pairs: 13 DSOs, 219 interface files and 836 libwacom data files.
Package hooks and configuration, systemd and D-Bus ownership/content match.
The additional GNU pair remains an audit input and is not admitted for return.

Ten source-worker cases exercise candidate admission and refusal. A further
[15 published-cohort plans](fedora-return-solver-results.txt) cover Core, KDE
and all admitted meta selections, with fresh runtime installation, upgrades,
mixed packages, reverse plans and weak dependencies disabled. Each disposable
root retains the exact signed kernel `7.3.1-106.fc46` selection, core, modules,
DTBs and boot tuple. These tests execute no RPM transaction and do not test a
blank-root installer. The plans use signed meta candidate `11239678`; final
`11239731` retains its payload, hooks, ownership and Requires with higher
child Releases. All eight final Control/meta binaries verify with the unstable
COPR key. No tablet installation, boot write, reboot or physical acceptance
was performed.

Cutefish remains excluded from this matrix because its separate provider
repository is not admitted by the Rawhide fixture.

Prepare the compatible signed Control manager before changing the Core
selection. New Core requires `uke-control(fedora-returns-v1) >= 1`. A tablet
transaction requires its separate device authorization and matched
kernel/updater/recovery record. With that preparation, review
`sudo uke-control brunch unstable --dry-run --json` before applying the plan.
The manager admits original Fedora repository IDs, vendor, owning source,
architecture, Fedora release and version floor. Only the declared old NEVRAs
may be removed, in the same transaction as their selected replacements.
Other removals remain errors. An unreviewed installed fork version is refused;
use the retained signed baseline as an explicit staging step before migration.

Keep Control 2.8 when reversing the desktop transition. The
[archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101)
now includes three retiring source RPMs, 12 signed fork binaries, the five
preceding meta binaries and the three compatible manager binaries.
[FEDORA-RETURN-SHA256SUMS](FEDORA-RETURN-SHA256SUMS) covers these 23 inputs;
all 24 added GitHub asset digests were checked. Unknown RPM outcomes retain
their operation records and cannot be replayed blindly. The guarded updater
continues to own automatic policy and the selected userdata boot profile.

The live inventory has 43 definitions: 26 active producers and 17 retained
definitions that refuse another build. The initial 36-source scope now has
25 active producers; the separately added `uke-o82-color-profile` accounts for
the remaining live producer. Five script fields changed in this step. Only
the meta source worker gains the development dependencies required for its
native solver checks. Other definitions, project settings and automatic
rebuild policy are unchanged. No definition or historical build was deleted.
These are source counts, not installed RPM or process counts.

The console GNU runtime stays on its existing capability and producer.
`uke-core-meta(fedora-cxx-runtime-v1)` is not provided: the console helper
policy and boot/power callers remain separate gates. Original Fedora desktop
helpers use the existing Fedora-root exception; project services, startup,
initramfs, recovery and installers gain no Python. `libaccounts-glib` retains
the upstream destructor correction missing from the reviewed Fedora source.
Firmware licensing, independent recovery packaging and physical kernel/DT
acceptance retain the limits recorded in the consolidation plan.
