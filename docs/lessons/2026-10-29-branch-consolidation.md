# UKE-BRANCH-20261029: preserve source families during branch consolidation

Date: 29 October 2026.
Environment: Fedora Rawhide x86_64 container; host tests only.

The former main (`173301a`) and Fedora-target change (`99779fd`) were joined
with reviewed device families (`025e53b`) and Control CLI/layout/charging work
(`f6241c1`). Speaker-orientation sources and host acceptance fixtures from
`23c8606` are retained alongside the integrated speaker controls. Component
source imports and older standalone branches overlap; merging their complete
old trees would remove current components and package corrections.

Resolution preserves Fedora-specific channel selection, the Cutefish selection,
current hardware/speaker versions, both Control feature sets and matching Core
requirements. The Core child override uses Control 2.3.0-1, matching its source
family. No kernel, firmware qualification, storage operation or tablet test is
implied. Native tests exposed missing host XFS/JSON development prerequisites;
the CI installation now includes those and the confined RGB policy dependencies.

All original remote branch tips are retained under archive/2026-10-29/ tags
before branch removal. Integrated ancestors remain reachable from main;
frozen source snapshots, superseded imports and isolated older experiments
remain recoverable from their exact tags. Archive retention is not production
admission. Physical charging, partition conversion, speaker operation and
new image acceptance still require their separately authorized device tests.
