# Battery longevity software contract

29 October 2026. Source preparation and isolated host tests for POCO Pad X1,
Fedora Rawhide AArch64 target. No tablet access, firmware command, installation,
service activation, reboot, suspend or physical charging test is performed.
The existing firmware, unqualified profiles and disabled service presets remain.

| Topic | Source behavior | Separate acceptance |
|---|---|---|
| Fault handling | A failed current observation clears increase dwell. Lease release reports permission activity without claiming restoration or cutoff. | Firmware/PMIC fail-safe response after service death, suspend or source change |
| Thermal policy | Decreases remain immediate; increases need a new stable dwell after a fault or identity/baseline change. | Calibrated sensors and exact OEM thermal/current limits |
| Daily 70/80 | Existing pair transport is reused with start 70/end 80; preview preserves firmware hysteresis within the band. | Installed KDE, actual cutoff/restart and lifecycle behavior |
| Heat-aware current | Existing qualified source/pack/thermal ceilings, positive current and bounded increases are retained. CV stays firmware-owned. | Actual heat, efficiency, load and battery-current measurements |
| Temporary full | Pure boot/source-bound window logic and 1-1440-minute preview; no persistence or timer. Apply is refused without a reviewed device-side return path. | Genuine autonomous expiry/return implementation and physical qualification |
| Hold and bypass | Online source, signed battery flow and reported Full/Not charging are separate observations. Bypass and electrical isolation remain unverified. | Independent power-path and battery measurements |
| Health | Forecast queries must match observed energy, capacity and conditions. Reordered EFC samples cannot rewind the cursor or double-count discharge. Coverage gaps remain explicit. | Pack/energy calibration, meaningful cycle semantics and aging evidence |

The observer adds no read: power-path disclosure uses already collected standard
attributes. Full or zero current does not establish battery isolation or effective
charge stop. Unknown status remains unknown. Reported hold is not promoted into
a qualified model input or charging command. Source events are included in
model continuity identity. Average telemetry remains unqualified and is not
silently substituted for the qualified model/control measurements.

The current optimizer never synthesizes a voltage command or forces discharge.
Cold/hot regions, gaps and zero-current bands require the existing reviewed
firmware protection path. No numerical electrical/thermal limit or gauge topology
is guessed from marketing capacity, product cell count or another board.

A graceful controller stop can report whether its owned lease revoke store
completed; a borrowed lease is not revoked. An applied setting may persist in
either case. Process death may produce no final event. Neither missing output,
lease expiry nor successful revoke implies restored defaults or electrical cutoff.
No blind restore SET is added on an error path.

Temporary full activation is unavailable in the native client. A preview checks
duration but does not query firmware, bind a live deadline, arm a timer, persist
an effective mode or install a worker. The pure planner's device-side return flag
is synthetic test input, never an operator toggle or runtime admission producer.
It rejects another boot/source, reversed time, invalid windows and an unqualified
active interval. At expiry it can propose the daily band; that proposal is not
evidence that hardware returned. A userspace-only timer is insufficient safety.

Keep physical calibration/protection gates open. Subsequent delivery requires
matched signed binaries, rollback and an attended validation window. Do not test
overheating, short circuits, wet connectors, forced deep discharge or unsupported
protocols on the tablet to obtain a software acceptance result.
