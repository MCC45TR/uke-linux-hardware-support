# Engineering lessons

| ID | Date | Finding | Validation |
|---|---|---|---|
| UKE-BAT-MATH001 | 2026-10-20 | Explicit pack basis and elapsed-time charge curves; reject stale/nonfinite estimates | Native/sanitizer fixtures and separate package/device receipts; physical calibration remains open |
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

## UKE-BAT-MATH001

- Date: 2026-10-20.
- Environment: native C++ host fixtures; separate Fedora Rawhide AArch64 tablet.
- Evidence: [calculation contract](../BATTERY-ENGINEERING.md), actual baseline
  NaN-query assertion failure, irregular-interval/taper/topology/EFC regressions.
- Finding: a query can inherit old current, and discontinuous windows can retain
  old future bands. Poll-count weighting also biases a charge curve. Explicit
  voltage/basis admission is needed before interpreting reported values as pack
  or average-cell quantities.
- Consequence: use current-cursor validation, full dependent-history invalidation,
  elapsed-time integration and reviewed optional topology/curve inputs. Default
  profiles remain unqualified; engineering output cannot enable a charger.
- Uncertainty: no individual cell telemetry, calibrated energy/current basis,
  active target readback, chemistry-specific aging model or lifetime prediction.
- Next validation: audit signed AArch64 payload, install observer only and inspect
  actual read-only output. Qualify physical model inputs separately from software.
