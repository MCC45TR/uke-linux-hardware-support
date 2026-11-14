# Native hardware ownership and original distribution packages

11 November 2026. POCO Pad X1 / Uke, Fedora Rawhide AArch64,
running kernel `7.3.3-22-unstable`, Control/TUI `2.17.0-1`.

The [ownership inventory](native-migration.json) covers the 33 current unstable
COPR producers and all 36 distinct installed `uke-*` names. It also identifies
same-name rebuilds such as ddcutil, Qt Virtual Keyboard, SELinux policy and
Tailscale. Producer absence does not mean an installed rebuild has disappeared.
These counts describe different populations and are not completion metrics.

## Ownership and migration

Kernel drivers own hardware protocols, topology, IRQs, resource lifetime,
protection and standard IIO/input/ASoC/DRM/media/power-supply interfaces. DT
describes actual hardware and supplier relationships. Keep desktop/session
policy, D-Bus, compilers, package transactions and private calibration out of
the kernel. Necessary Uke session integration belongs in the existing Control
package, with event/demand activation and no additional bridge RPM.

Every inventory entry identifies its destination, original package candidates
and remaining work. An empty original-package list means that a replacement
has not been established; it does not authorize removing the function.
Firmware, essential build checks, image tooling and recovery retain their
necessary roles until separately qualified replacements exist.

The first active item remains battery/charging and original UPower. Core22's
standard-provider dependency was built and compared with Core21, but remains
uninstalled. Stock UPower still differs on unknown battery health and recovery
after late capacity data. Do not fabricate kernel health or capacity to hide a
consumer bug. Native event protection and real 80/70 hysteresis/full-charge
termination remain open. The sensor transition separately requires valid
magnetic heading and lifecycle equivalence; a game quaternion is not a compass.

Mesa, compiler bitcode and VA-API remain userspace implementations. Native DRM
or V4L2 enumeration does not establish original-package rendering, compute or
video parity. Qt's original package currently lacks the installed key-sound
implementation. Upstream/distribution corrections or supported standard
integration must preserve those features before replacements are selected.

## Read-only audit

Use a complete public package list in the format produced by the
`uke-copr-maintenance` skill's `list-public.sh`. Its `meta.complete` marker,
project identity and unique producer IDs/names are checked before any report
is written. Use fresh output filenames:

```sh
bash packaging/families/host/native-migration-audit.sh snapshot /tmp/uke-rpms.tsv
bash packaging/families/host/native-migration-audit.sh audit \
  /tmp/complete-copr-packages.json /tmp/uke-rpms.tsv /tmp/uke-migration.json
bash packaging/families/tests/native-migration.sh
```

The audit correlates installed RPMs by owning source, including split RPMs and
historical installed sources. It does not infer package ownership from a name
prefix alone. Source/vendor fields are discovery hints, not signature proof.
No credentials, hardware identifiers or calibration are read. Raw snapshots
remain private; generated reports omit vendor identities.

Exit 0 means every selected source and producer is mapped. Exit 2 writes a
report identifying unmapped packages/producers. Malformed, partial, duplicate
or wrong-project input fails without creating a report. Coverage never grants
retirement: all functional/signature/DNF/physical acceptance fields remain
false and the tool performs no package or remote mutation.

## Preventing regressions

The retired-source guard rejects project rebuilds of PipeWire, libwacom,
GStreamer, accessibility and GNU runtime families, including their renamed
subpackages. Standalone generation, umbrella/archive composition and direct
native-runtime preparation consult the same policy before source/build work.
Original Fedora packages used as dependencies are unaffected. Existing
UPower/sensor/Mesa providers are not retired by this guard.

The source tests exercise an unprefixed rebuild with a distribution vendor
label, a split runtime whose source still has other users, a missing producer,
partial/duplicate/wrong-project inventories, and retired-source entry points.
Umbrella tests stub all build/download tools and prove refusal before they run.
Tests require only Bash, jq and standard file tools; no native compilation or
RPM installation occurs on the tablet. The family source CI runs this suite.

## Retirement acceptance

For each component, preserve source revisions, licenses/attribution and the
exact remaining delta; compare Uke OEM, applicable shared-IP donors, upstream
and distribution sources under the workspace donor policy. Record separate
source, COPR native build, signed RPM, real DNF transition and hardware results.
Qualify cold boot, restart, suspend/resume, hotplug, errors and idle behavior.
Preserve settings, dependency/file ownership, automatic staging and recovery.
Only then remove redundant producers and verify both API absence and refreshed
repository metadata. The Desktop tracker and kernel queue remain authoritative
for execution; this inventory does not reserve a kernel revision or authorize
stable promotion.
