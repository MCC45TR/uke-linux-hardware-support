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
