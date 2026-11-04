# Kernel rollback policy

The 4 November 2026 development candidate adds a persistent, root-owned
`/etc/senemos/rollback.conf`. Missing configuration means `enforced`.
RPM upgrades preserve administrator changes with `%config(noreplace)`.

| Command | Boot health | Automatic fallback |
| --- | --- | --- |
| `uke-control set rollback-enforced` | Require the existing continuous health qualification | Return to the accepted cohort when qualification fails |
| `uke-control set rollback-permissive` | Observe health; keep an unhealthy trial unaccepted | Disabled |
| `uke-control set rollback-off` | Skip automatic acceptance | Disabled |

`uke-control rollback status --json` reports the persisted mode. Mode changes
do not select a slot or request a reboot. Run setters as root. The default
for end users remains enforced; permissive and off require explicit selection.
An off trial remains a trial rather than becoming an accepted healthy baseline.

Permissive/off allow an explicit checked retry of a previously failed release.
Automatic package transactions retain the failed-release guard. All modes
retain candidate package/digest checks, exact device and UFS identity, power
requirements, early-firmware checks and transactional slot write/readback.

An explicit off-mode update may use the recorded accepted slot hash and exact
running release after an administrator removes the old kernel packages. The
old partition is retained, but its missing modules prevent a qualified full
rollback. Enforced updates still require the complete accepted RPM cohort.
Normal same-family updates keep the installed accepted cohort for rollback.
Switching exclusive families removes the opposite family's RPMs; development
off mode must be selected before doing so. Returning to enforced operation
requires a healthy installed cohort and acceptance on a subsequent boot.

COPR producers retain the published 1.8.40 boot integration and 2.8.1-12 control
sources by SHA-256, overlay the policy changes, and run the native fixtures in
COPR. No native compilation is performed on the tablet. Source/build results
do not establish physical health or recovery from an early bootloader hang.
