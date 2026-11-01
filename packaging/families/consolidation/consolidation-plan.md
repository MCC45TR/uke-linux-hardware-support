# Source consolidation

1 November 2026. Fedora Rawhide 46, AArch64. The live COPR inventory contains
36 producer definitions. GitHub main is `bb2899e`; producers retain independent
source pins. The kernel source recipe is pinned to `d86aea3e8cc2f90ac101d03b81560d8e1eab38cc`
and build 11239296 is running at this snapshot. It is not part of these source
consolidations.

The six profiles in `../integrations.json` reuse the existing source composer.
They take complete, checksum-pinned successful source RPMs, retain component
versions, licenses, checks, dependencies, scriptlets and binary names, and
increase every child Release. A source grouping creates no new runtime RPM,
mandatory dependency or service. Each profile has its own revision; changing
the common converter does not request unrelated rebuilds.
Increase the source Release for every rebuild: it also determines each child's
increment above its pinned baseline. Do not reset it while retaining the same
baseline versions. The sensor source retains the existing optional
`uke-ssc-fileservice-debuginfo` and `uke-ssc-fileservice-debugsource` identities.

| Source group | Existing producers | Reduction |
|---|---|---:|
| uke-meta-integration | core-meta, desktop-metas | 1 |
| uke-audio-integration | displayport-audio, speaker-controls, speaker-orientation | 2 |
| uke-sensor-integration | ssc-fileservice, front-color, proximity-capture | 2 |
| uke-power-integration | battery-observer, adaptive-display | 1 |
| uke-video-integration | vaapi-v4l2, video-session | 1 |
| uke-desktop-extras-integration | plymonth, material-decoration | 1 |

The candidate active-producer count is `36 - 14 + 6 = 28`. The plan's 26
requires two further merges which this source review does not admit. The
current generic firmware source explicitly says its file-level redistribution
review remains incomplete; it cannot absorb the independently attributed
Apache-2.0 speaker preset. Recovery remains separate because its large image
archive and source offer should not depend on a Qt/KWin decoration rebuild.
These are documented lifecycle and licensing boundaries, not completed merges.

The conditional Fedora returns are a separate step. Removing five policy-only
forks would make this design 23 active producers, or 21 if the two deferred
merges later qualify. The existing original-Fedora desktop exception is not
permission to introduce Python hardware/startup services or recovery payloads.
Each replacement still needs a signed API/ABI, file-ownership and dependency
comparison. A source count reduction does not reduce binary RPMs or processes.

No producer definition or old build is deleted during candidate builds. Old
signed binaries and source pins remain the rollback inputs. A final live
inventory must distinguish retained definitions from active producers.
