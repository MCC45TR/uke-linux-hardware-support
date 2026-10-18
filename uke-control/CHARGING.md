# Charging policy source — 18 October 2026

Environment: Uke Control source, 18 October 2026. The shared backend and CLI
include metadata-only policy previews. Synthetic host fixtures pass; no charging
write or physical policy acceptance has been established.

The requested presets are `charge80` (autonomous end threshold 80%), `hold80-90`
(autonomous start 80%, end 90%), `slow` (a qualified pack-specific current), and
`automatic` (a qualified thermal/current/voltage envelope). `firmware` expresses
the existing firmware policy without requesting AP control. Targets and rates
are separate in the C++ interface, so a qualified backend can combine an 80%
cap with slow charging. No default electrical limits are supplied.

`hold80-90` retains charge/hold demand within the band and clears that memory
after a fault, epoch change, input gap or policy change. A restart within the
band leaves the decision to autonomous firmware hysteresis. Neither policy
commands discharge to reach the target nor represents zero USB input current
as a charging cutoff, battery bypass or electrical stop.

The pure planner takes qualified battery-side current, voltage, temperature and
available power after source conversion loss and system load. It intersects
pack, path, firmware and thermal current ceilings; slow mode adds its own
qualified ceiling. Warm-band reductions take effect in the next proposal;
increases require stable-band dwell and bounded current steps. Missing or
invalid telemetry, expired samples, non-increasing timestamps, suspend, faults,
identity changes and unqualified controls block proposals. Thermal gaps,
inhibit bands and excessive battery voltage require firmware protection and
produce no current or voltage proposal. There is no VBUS negotiation.

Proposals are never applied state. A future actuator must obtain fresh readback,
verify the same source/firmware identity, serialize with the existing firmware
and gadget owners, apply supported controls and separately establish effect.
Planner history contains prior recommendations only; it cannot prove a setter
ran, maintain a physical cutoff or substitute for autonomous protections.

## CLI integration

The shared C++ backend exports the policy planner. The CLI exposes these
metadata-only commands; `make check` runs the synthetic policy fixture:

```sh
uke-control charge modes
uke-control charge plan charge80
uke-control charge plan hold80-90
uke-control charge plan slow
uke-control charge plan automatic
```

These calls return metadata only and perform no power-supply GET. `charge mode
MODE` is an explicit unsupported apply interface: it returns exit 69, `applied:
false`, `effective: null`, with no write or saved policy. It cannot report a
successful selection while the actual actuator is missing. The existing
standard `charge limit` handler remains a separate interface; do not route it
through the proposal planner or infer control qualification from sysfs names.

The synthetic host fixture checks SOC boundaries/hysteresis, source-power and
thermal ceilings, slow/automatic composition, invalid/stale inputs, identity
changes and unsupported apply. Its electrical numbers are test data, never a
production Uke profile. Host and package checks remain separate from tablet results.

The current kernel exposes no standard charge threshold. The exact Uke OEM
threshold opcode has no-send success and ACK/effect ambiguities, and its thermal
property has conflicting level/current semantics. This module therefore
supplies no firmware actuator. Battery health labels, reported SOH, reported
cycles, qualified capacity retention and observed discharge EFC remain separate.
See the central `UKE-CHARGING-CONTROL-ABI-2026-10-18.md` research record for exact
OEM/donor pins and the outstanding backend evidence.
