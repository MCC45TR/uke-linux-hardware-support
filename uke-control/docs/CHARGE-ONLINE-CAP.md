# Explicit reapplication of the existing charge cap

11 November 2026. Source preparation; native build, RPM delivery and physical
charge cutoff remain separate gates. No service or runtime package is added.

`sudo uke-control charge apply-cap80-online` invokes the matched kernel's
root-only `uke_charge_apply_existing_cap_online` operation once. The kernel
requires the already-enabled exact70/80 band, present/Good battery, online
input, fresh SOC at/above80, no latched fault and an unchanged transport epoch.
It never raises the ceiling or exposes an arbitrary charging operation.
Older kernels without this endpoint refuse; there is no fallback to the
offline command or another writer. The original offline command is retained.

The existing Control/helper lock and context checks still apply. No settings
GET is inserted immediately before the write: the matched kernel owns the
fresh diagnostic queries and rate gate. Sysfs store errors retain their errno;
short stores report their byte count without inventing an errno. Failed,
interrupted and short writes each cause exactly one attempt, no retry or undo.
A return0 means the matched request/reply and configuration checks completed;
`applied` and `physical_charge_stopped` remain null. It does not establish
that current stopped, the charger autonomously keeps the band, or that the
operation may safely be replayed after a timeout or restart.

Qualified-provider intent is preserved so the legacy threshold restorer cannot
silently overwrite it. This is an explicit diagnostic bridge in the existing
Control package, not an automatic boot/reconnect policy. Before electrical
qualification, retain the firmware protections, current limits and recovery
route. Test80 cutoff/70 resume, current/counter/status/voltage, temperatures,
reconnect/suspend/DSP restart and full100 termination separately.
