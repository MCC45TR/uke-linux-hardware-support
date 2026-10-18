# Engineering lessons

| ID | Date | Finding | Validation |
|---|---|---|---|
| UKE-INITIAL-001 | 2026-10-04 | Initial source/packaging scope established from the Nabu COPR inventory | Manifest checks only; package and physical validation open |

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
