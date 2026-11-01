# Battery health reporting follow-up — 18 October 2026

Environment: source preparation against the corrected observer at `b6d1680`.
No test, build, daemon activation, tablet operation or calibration write was
performed by this task. The integration coordinator owns validation and install.

Snapshot schema 1 retains `health.capacity_retention_percent`,
`health.learned_capacity_qualified`, `observed_efc`, `efc_coverage` and existing
field meanings. The additive `health_provenance` object separates the reported
health label, vendor SOH and cycle counter from the qualified retention model
and observed-only EFC. Each reported source references its existing
`linux_power_supply` field; that field contains validity, errno and receipt
time. No additional GET is introduced. No upstream firmware origin or physical
protection is authenticated by a sysfs label.

Only `cycle_count` gains a nonnegative range check. A reported integer zero is
preserved with `validity: unqualified`; it is not an out-of-range measurement or
proof that the battery has never cycled. Negative, oversized, missing, read-error
and stale values remain unavailable. All existing bounds for other properties
remain unchanged. `effective_cycles` stays null and `semantics_qualified` stays
false; no physical cycle qualification or counter reset is added.

The synthetic collector fixture now covers zero, positive, negative, overflow,
missing and transport-quiesced cycle inputs, reported SOH versus retention,
health-label versus safety status, and legacy snapshot paths. The coordinator
must run `tests/battery-collector.sh` against the exact commit and retain the
host result separately from tablet acceptance. The service stays disabled and
the existing package/provider admission remains in force.
