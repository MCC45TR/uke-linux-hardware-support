# Engineering lessons

| ID | Date | Finding | Validation |
|---|---|---|---|
| UKE-INITIAL-001 | 2026-10-04 | Initial source/packaging scope established from the Nabu COPR inventory | Manifest checks only; package and physical validation open |
| UKE-TOUCH-PERF001 | 2026-10-18 | Reuse positional panel-state reads | Controlled tablet CPU and syscall comparison |
| UKE-TOUCH-CMD001 | 2026-10-18 | Poll pending commands through wide-VFP startup | Signed RPM and two seven-mode reset sequences |
| UKE-SSC-MAT001 | 2026-10-18 | Preserve nonzero matrices whose entries sum to zero | Actual-source host regression fixture; tablet comparison pending |

## UKE-INITIAL-001

- Date: 2026-10-04.
- Environment: project-owned Git repository; no tablet execution.
- Evidence: component manifest, source policy and acceptance roadmap.
- Finding: Nabu package roles can organize Uke work; their hardware assumptions
  cannot establish Uke behavior.
- Consequence: package admission requires Uke sources and isolated payload tests.
- Uncertainty: no functional package exists at this initial scaffold stage.
- Next validation: complete the manifest's component-specific gates.

## UKE-TOUCH-PERF001

- Date: 2026-10-18.
- Environment: Fedora release-9 kernel, original KDE, 180-frame/s Uke touch stream.
- Evidence: three-second syscall capture and ten-second idle CPU delta; current
  panel provider emits an atomic decimal generation and readiness line.
- Finding: five ifstream opens per frame caused 2,702 open/close pairs in the
  three-second sample. The existing provider descriptor can serve fresh pread
  checks without repeating path lookup or changing IRQ/SPI/matrix semantics.
- Consequence: release-8 source retains every transfer guard on the open fd.
- Validation: actual parser, provider rereads, invalid/closed reads, generation
  races and recovery-backoff fixtures; signed package and tablet checks follow.
- Uncertainty: syscall reduction is not a measured power or physical touch result.
- Next validation: compare target CPU/syscalls, panel-reset recovery and input.

## UKE-TOUCH-CMD001

- Date: 2026-10-18.
- Environment: Uke release-10 kernel, O82 144 Hz base with opt-in 30 Hz AVR floor;
  release-8 native touch service and a guarded native SPI latency helper.
- Donor evidence: the existing Uke OEM BF read path waits 35 ms and performs
  five write/wait/read attempts at 20 ms. The normal reset-ready poll separately
  permits 500 ms. No command payload, scan mode, firmware or calibration changes.
- Observed result: cold wide-AVR activation repeatedly times out on read command
  1. After the same pinned RAM image is reloaded under held wide AVR, a single
  query remains pending for about 345 ms and then returns operating mode 1.
  A later query completes in 10.7 ms; the narrow 120 Hz floor control takes 5.4 ms.
- Consequence: the old 135 ms query window can end before firmware command
  readiness. Release 9 sends once and polls at 20 ms within the existing 500 ms
  readiness budget. Every SPI transfer retains the panel-generation guard.
- Validation: actual command fixtures cover delayed ACK, one write, bounded
  failure, transfer failure and a panel reset while the command is pending.
- Uncertainty: the latency observation does not qualify panel scan cadence or
  physical finger input. It does not establish that retransmission alone caused
  the delay. Package and repeated cold-AVR recovery acceptance remain pending.
- Next validation: install the signed RPM and repeat fixed and wide AVR lifecycle
  tests across every admitted base mode, then check idle CPU and logs.

## UKE-SSC-MAT001

- Date: 2026-10-18.
- Environment: libssc 0.4.4 source pin, release-3 Uke runtime; current upstream
  main `4e4c6c8cc702a754bd700506c01961aaf971a23b` retains the same predicate.
- Evidence: `src/libssc-sensor.c` adds float entries into an integer and treats
  a zero sum as an all-zero matrix. No OEM device matrix is transferred.
- Finding: a valid nonzero rotation can sum to zero and be replaced incorrectly.
  Integer truncation also loses fractional components during the check.
- Consequence: release-4 source tests each entry for zero and preserves every
  nonzero supplied matrix. The existing all-zero warning and identity fallback
  remain; warnings now identify the sensor data type.
- Validation: the actual upstream fallback block fails the 120-degree X-rotation
  fixture; the patched block preserves it, fractional rotations and each nonzero
  element while retaining all-zero fallback. All five source patches apply
  without fuzz. Source-factory negative controls pass.
- Uncertainty: the Uke boot emits two old generic warnings; their sensor types
  and matrix values are not yet correlated. This host regression does not prove
  that either observed warning is a false positive or validate physical axes.
- Next validation: signed package/payload audit, stationary runtime comparison
  and identification of the remaining firmware metadata warnings. Physical
  rotation and calibration remain separate acceptance work.
