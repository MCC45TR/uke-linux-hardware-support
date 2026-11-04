# UKE-QT-SETTINGS-PLAN-20261104: merge command guides into user topics

Date: 4 November 2026. Environment: POCO Pad X1, Fedora Rawhide aarch64;
Qt 1.1.0-2, Control/client 2.8.1-23, battery observer 1.4.1-7,
running kernel 7.3.1-7200-unstable.

The user requested comprehensive planning before further Qt changes. The
installed metadata contains 129 actions in 16 categories, including 78 reads.
The retained Qt source has nine areas and a generic guide per SDK action;
choice/integer fields are uniformly combo/spin controls. This creates repeated
status/settings routes and exposes internal operation boundaries to users.

The [proposal](../../uke-control-qt/docs/SETTINGS-UX-PLAN.md) defines six everyday
topic categories, secondary Help/Advanced, summary tiles, semantic controls,
availability states and justified graphs. Its action map covers all 129 installed
actions exactly once in 38 feature ownership groups. These are ownership groups,
not a requirement for 38 pages. Main has an older Qt layout than the retained
1.1.0-2 branch; future implementation must reconcile the correct baseline.

Installed flashlight metadata explicitly describes only two lit levels. A
percentage slider would imply unsupported granularity. Temporary full charge
is preview-only and cannot become an enabled timer. Existing writes are terminal
handoffs; true immediate switches need a reviewed execution/outcome path rather
than an optimistic UI change. Hardware/readback approvals remain authoritative.

A real read-only battery request returned coherent, provider-verified reading
objects with source_unqualified status. This permits presentation of reported
basic values, not qualified health/ETA claims. The captured API has no time-series
array or application energy attribution. Existing model checkpoints are not a
chart archive. A subsequent read-only original UPower inspection found composite
DisplayDevice history unavailable but actual battery HasHistory=true. A 24-hour
request returned 25 records, including 12 unknown-state markers, in nonascending
order over 76,111 seconds. Prefer this existing history through a Qt read-only
adapter, preserving unknown markers/gaps and actual time coverage; do not add a
duplicate recording service. A bounded session series is a fallback. Per-app
energy remains unavailable. Original KQuickCharts is installed, but no new chart
rendering or implementation was tested in this planning task.

Validation was metadata/source analysis, read-only battery/UPower observations
and exhaustive action-map checks only.
No application source, package, boot metadata or calibration was modified; no
native compilation, DNF transaction, slot selection or reboot was requested.
Next validation is design review followed by scoped Qt implementation and COPR
tests. Planned controls and graphics are not delivered functionality.
