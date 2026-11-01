# Source consolidation

1 November 2026. Fedora Rawhide 46, AArch64. The
[current source snapshot](live-inventory-20261101.json) contains all 43 COPR
definitions: 23 active and 20 retained without another build. Its empty offset
43 page confirms the complete listing. Recipe hashes, exact pins and current
main/TEST refs remain separate from installed-device evidence.

The initial audit covered 36 definitions. Its source recipes, binary baseline
and earlier refs remain in [inventory.json](inventory.json) and
[package-map.csv](package-map.csv). The
[current route table](current-source-routes.csv) covers all 43 definitions.
The initial kernel recipe used `d86aea3e8cc2f90ac101d03b81560d8e1eab38cc`;
it was excluded from the source consolidations. The current independent
kernel recipe uses `8983163763f708ec940b349ebb81ab43cc3725c3` and is not
qualified by the runtime fixture's retained kernel 106.

The [Control follow-up](control-family-results.md) combines its existing source
with the meta-selection and native power/display integrations. It retains all
12 binary names and removes two active producers without creating a new source
definition. Five grouped producers remain active; the two initial integration
profiles stay as pinned historical inputs and component maintenance recipes.

The seven profiles in `../integrations.json` reuse the existing source composer.
They take complete, checksum-pinned successful source RPMs, retain component
versions, licenses, checks, dependencies, scriptlets and binary names, and
increase every child Release. A source grouping creates no new runtime RPM,
mandatory dependency or service. Each profile has its own revision; changing
the common converter does not request unrelated rebuilds.
Increase the source Release for every rebuild: it also determines each child's
increment above its pinned baseline. Do not reset it while retaining the same
baseline versions. The sensor source retains the existing optional
`uke-ssc-fileservice-debuginfo` and `uke-ssc-fileservice-debugsource` identities.

The initial six integrations were:

| Source group | Existing producers | Reduction |
|---|---|---:|
| uke-meta-integration | core-meta, desktop-metas | 1 |
| uke-audio-integration | displayport-audio, speaker-controls, speaker-orientation | 2 |
| uke-sensor-integration | ssc-fileservice, front-color, proximity-capture | 2 |
| uke-power-integration | battery-observer, adaptive-display | 1 |
| uke-video-integration | vaapi-v4l2, video-session | 1 |
| uke-desktop-extras-integration | plymonth, material-decoration | 1 |

The initial active-producer count was `36 - 14 + 6 = 28`. The plan's 26
requires two further merges which this source review does not admit. The
current generic firmware source explicitly says its file-level redistribution
review remains incomplete; it cannot absorb the independently attributed
Apache-2.0 speaker preset. Recovery remains separate because its large image
archive and source offer should not depend on a Qt/KWin decoration rebuild.
These are documented lifecycle and licensing boundaries, not completed merges.

The [four Fedora returns](gnu-return-results.md) reduce the initial scope from
28 to 24 active producers. The independently added color-profile source makes
the count 25 before the Control follow-up reduced it to 23. Original Fedora
helpers on the Fedora root, including GNU
C++ console helpers, were approved on 1 November 2026. The libaccounts-glib
source retains a real destructor correction missing from the reviewed Fedora
source. The original
21-source proposal also depended on both deferred merges and all five Fedora
equivalents; it is not the current result or a required quota.
Project Python, Python hardware/startup services and Python in recovery,
initramfs or installers remain excluded.
Each replacement still needs a signed API/ABI, file-ownership and dependency
comparison. A source count reduction does not reduce binary RPMs or processes.

No producer definition or old build is deleted during candidate builds. Old
signed binaries and source pins remain the rollback inputs. A final live
inventory must distinguish retained definitions from active producers.

Source maintenance stays explicit. To change one integration, prepare and
validate that component's complete source recipe, archive its source offer,
update only that profile's URL/hash/build identity, and increase the profile
Release. The manifest preserves the original COPR URL alongside the durable
GitHub archive URL. Never silently follow main or reuse a published child
NEVRA. Unchanged integrations and large upstream forks need no rebuild.

The live snapshot records kernel/main divergence and separate builder and
Control comparisons at capture time. Preserve the actual source/DT/modules
tuple; a main branch ref does not identify the installed kernel. Independent
kernel and builder updates were preserved. `uke-linux-unstable` denotes the
COPR project; its recipes use explicit source pins.

The first routing phase produced 42 definitions, with 28 admitted producer
scripts and 14 superseded scripts retained for history and rollback. Its
20-script comparison is retained in routing-receipt.json. That phase reduced
source maintenance by eight producers without reducing binary RPMs or forks.
The later [desktop return result](fedora-return-results.md) records the three
first three fork withdrawals. The subsequent [GNU result](gnu-return-results.md)
records the fourth return, compatible callers, worker dependencies and unchanged
unrelated fields. No historical definition or build was deleted.
