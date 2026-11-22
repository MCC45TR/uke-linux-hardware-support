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

## UKE-CORE-003 — prevent transitive Python in the solver

- Date: 2026-10-05.
- Environment: Rawhide AArch64 full desktop transaction, not a tablet.
- Evidence: optional Fedora bindings and installed build/documentation utilities
  introduced six Python packages into the first complete Plasma selection.
- Consequence: release 2 explicitly conflicts with the Python ABI and interpreter
  packages. Dependency changes fail solving rather than silently relaxing the
  project's target policy. The console selection still needs an extracted
  complete-runtime audit; these conflicts cannot detect undeclared scripts.
- Next validation: corrected native dependencies, signed transactions and payload audit.

## UKE-CORE-004 — inherited scripts require a source capability

- Date: 2026-10-05.
- Environment: complete installed Rawhide AArch64 root under QEMU userspace.
- Evidence: the installed package-name guard passed but the root scan found 15
  Python/PYC debugger helpers in the already installed Fedora libstdc++.
- Consequence: release 3 requires the source-built native libstdc++ capability,
  retaining interpreter conflicts and ordinary dependency solving. The library
  replacement must remove its old RPM-owned helpers during upgrade.
- Uncertainty: source capability, signatures, complete root and lifecycle are
  separate gates; no boot or console hardware result is inferred.
- Next validation: native source build followed by actual signed package
  installation/upgrade/removal and full inherited-root inspection.
