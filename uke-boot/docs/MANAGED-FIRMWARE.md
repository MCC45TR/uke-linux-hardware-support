# Managed private firmware before kernel activation

5 November2026, POCO Pad X1. Kernel12204 boots with private MCU/CDSP files absent
from its versioned directory although the previous12203 directory contains
them. Source/build success does not prove firmware delivery or physical support.

The native updater requires provider0.2-8 or later. For permanent-PAS cohorts
beginning at7.3.1-11003-unstable and later kernel versions, it runs
`--prepare-target RELEASE` and `--verify-target RELEASE` before candidate image,
slot-selection or candidate-state callbacks. Root dry checks only verify; their
stock comparison requires the provider's read-only block/loop access. Cached
GUI status is unchanged. Accepted fallback/current-slot restore and older
recovery cohorts retain their previous paths. Automatic staging and the existing
rollback preference are preserved, with enforced package defaults.

The pending-trial guard prepares/verifies the actual recorded candidate before
reporting it ready for reboot. The boot acceptance callback also prepares and
verifies the running cohort, including an unchanged default selection. A
different running baseline is checked before automatic acceptance; a dry
preflight only verifies it. Native activation covers automatic transactions,
manual application and selection, and startup staging. Existing private-color
preparation, RTC restoration, validation and failure recovery remain in place.
The provider publishes authenticated device-local files atomically, refuses
differing existing data and starts no processor, trusted app or peripheral.
Kernel/DT cannot publish these private stock files; this existing native
integration is necessary. No calibration or OEM firmware enters the RPM source.

Separate RPM triggers provide no failure barrier. The
[RPM6.1.0 transaction implementation](https://github.com/rpm-software-management/rpm/blob/rpm-6.1.0-release/lib/transaction.cc)
runs existing and newly installed transaction triggers in separate passes.
[Trigger execution](https://github.com/rpm-software-management/rpm/blob/rpm-6.1.0-release/lib/rpmtriggers.cc)
continues after accumulating a script error. Priorities are sorted within a
pass; the manual's general unsupported-priority note is not a substitute for
this source inspection. These upstream sources are behavioral evidence; no
RPM implementation is imported into the original MIT C++ gate.

The source factory retains the349KB qualified1.8.95 source capsule under
`referances/qualified-source/` with its exact hash and provenance. This avoids
depending on expiry of older COPR artifacts. Every other native source, test,
RTC/policy file and existing admission gate is verified unchanged. New callback
checks compare the original color-only boundary and inject preparation,
verification and private-color failures under GCC/Clang and sanitizers.
The actual production `boot_userdata()` flow is exercised with unchanged and
different selections, enabled/disabled staging and legacy recovery. Its original
1.8.96 ungated acceptance callback must fail the new firmware assertion.
Full package checks, signed repository delivery, preserved device state and
physical camera/input/DSP/accessory operation remain separate acceptance gates.
