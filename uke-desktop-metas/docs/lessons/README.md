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

| UKE-PACKAGE-002 | 2026-10-05 | Implemented real source/RPM rules from independently scoped Uke inputs | Source validation; target and native results recorded separately |

## UKE-PACKAGE-002

- Date: 2026-10-05.
- Environment: official Fedora host source tools; no tablet execution.
- Evidence: package specs, Make source factory and explicit Uke readiness records.
- Finding: the Nabu package roles are reusable, while firmware, panel, services
  and boot integration require their own Uke evidence. Shared Fedora software
  remains upstream; package data and dependency selections have explicit scope.
- Consequence: only real source families enter automatic COPR compilation.
- Uncertainty: source-generation checks do not establish target closure, native
  binary acceptance, graphical rendering or physical operation.
- Next validation: collect native COPR and signed target transaction results in
  distinct reports, then separately qualify hardware.

## UKE-DESKTOP-META-003 — require inspected native variants

- Date: 2026-10-05.
- Environment: complete Rawhide AArch64 desktop dependency and RPM payload audit.
- Evidence: Python dependencies in five Fedora source families, plus two undeclared
  Dolphin migration scripts, failed the original desktop acceptance gate.
- Consequence: release 2 requires the core policy guard and six explicit native
  runtime capabilities. The source rules for those variants remove optional
  Python consumers and provide native C++ configuration migrations.
- Uncertainty: source generation does not prove a complete working runtime.
- Next validation: signed native builds and complete install/upgrade/removal audit.

## UKE-DESKTOP-META-004 — include the GNU C++ base dependency

- Date: 2026-10-05.
- Environment: 603 selected RPMs plus pinned inherited Rawhide AArch64 base.
- Evidence: selected inputs and meta release-1 to release-2 lifecycle passed;
  the full installed root failed on Python GDB helpers inherited from libstdc++.
- Consequence: release 3 requires core release 3 and a seventh native runtime
  capability. The bounded earlier results remain valid, while that root remains
  rejected and cannot be labeled Python-free.
- Uncertainty: package acceptance does not exercise a graphical session,
  decoration rendering, Uke kernel boot or physical support.
- Next validation: signed corrected closure, native fixtures, complete root,
  actual release upgrade and admitted package removal.

## UKE-DESKTOP-META-005 — withdraw derivative KDE dependency selection

- Date: 2026-10-05.
- Environment: explicit owner instruction, source profile/spec and COPR policy.
- Evidence: the owner prohibits cloning KDE desktop applications. Plasma and
  Dolphin variant source records and seven completed builds were removed after
  exact local archival. Both source targets now fail before archive retrieval.
- Superseding correction: release 3 no longer requires seven runtime
  capabilities or supplies a KDE selection. It requires core release 3, carries
  policy/readiness data and obsoletes the old `kde-plasma-uke-meta` package.
- Consequence: use original distribution applications; their current Python
  components block full graphical admission under the separate target rule.
- Uncertainty: metadata compilation and console transactions do not establish
  a graphical session, device boot or physical support.
- Next validation: signed data-only RPM, actual earlier console-meta upgrade,
  complete inherited-root audit and removal.
