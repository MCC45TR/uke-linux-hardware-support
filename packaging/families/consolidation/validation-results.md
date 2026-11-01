# Validation results

1 November 2026. Local Fedora Rawhide AArch64 source editing on Uke. No native
compilation, RPM build, package transaction or hardware test runs locally.

| Scope | Command / evidence | Result |
|---|---|---|
| Source | `bash -n packaging/families/host/compose.sh` | Passed |
| Source | `cd packaging/families && bash tests/split-spec.sh` | Passed, including preserved child Version/Epoch, incremented Releases, exact ISA dependency and unchanged legacy Obsoletes bound |
| Source | `git diff --check` | Passed |
| Inventory | Official COPR API, all 36 entries and empty offset 36 page | Captured in inventory.json; pins differ from main |
| Source archive | SHA256 of 17 successful component SRPMs | Recorded; 14 selected, firmware pair and recovery deferred |
| Native build | Six new integration sources | Pending COPR |
| Signed RPM | Identity, ownership, signatures and payload audit | Pending native outputs |
| Solver / state migration | Fresh, skipped, mixed and reverse transactions, optional selections and service/config states | Pending isolated fixture |
| Device | Installation, boot, touch, audio, suspend, sensors, charging | Not performed |

A green source check does not establish package, solver or physical acceptance.
