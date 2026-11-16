# UKE-FIRMWARE-FULL734-20261116: bind fresh-root admission to a successful build

Date: 16 November 2026. Environment: separate development PC, complete
managed firmware 11 source and cumulative kernel 7.3.4-2 build 11332265.

Existing future inactive-target preparation already handles the newer ABI.
A fresh root running that ABI additionally needs its exact source in the
current idle-consumer allowlist. Version parsing alone does not grant that
admission. The prepared change adds only 7.3.4-2 and source 385b4893; it retains
the accepted kernel 28 tuple and all previous entries, factory MCU validation,
read-only stock authentication, active-consumer refusal and additive publication.

The source worker requires the referenced full build to have succeeded and
its actual worker to name the exact complete source commit before assembling
helper 12. It preserves all six managed firmware 11 inputs and checks the complete recipe
inverse. The native managed fixture adds exact/current success, wrong source
and neighboring release refusal, active and verification-only refusal, four
idle checks and complete-family idempotence. Existing fixtures remain intact.

PC checks cover source application and the complete recipe inverse only.
Full-kernel success, native helper tests, signed DNF and first-boot operation
remain separate results. This preparation starts no firmware or controller.
