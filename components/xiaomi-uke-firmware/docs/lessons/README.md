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

## UKE-FIRMWARE-002 — file-level admission remains required

- Date: 2026-10-05.
- Environment: public reference inventory and pinned source archives; no device write.
- Evidence: `reports/ADMISSION-2026-10-05.json`; Uke donor DT firmware descriptions
  and the archived linux-firmware inventory.
- Finding: a Nabu package name, Uke device-tree description or generic Qualcomm
  redistribution license does not admit unspecified Uke OEM files. No complete
  source/path/hash/license/kernel-request map has passed at this checkpoint.
- Consequence: no empty firmware RPM or Nabu blob copy is published as Uke support.
  The repository and validation workflow remain available; COPR source admission
  waits for the actual file allowlist.
- Uncertainty: this result does not assert that no redistributable Uke firmware
  exists; it records the missing project evidence.
- Next validation: identify and audit the exact payload and license for each file.
