# Explicit replacement of a development trial

4 November 2026. Source candidate; remote qualification and physical validation
remain required. The normal stable kernel family is updated only on an explicit
request, after physical validation. Camera development uses the unstable family.

`uke-kernel-update check-trial BUNDLE` inspects an installed, RPM-verified
unstable bundle. `replace-trial BUNDLE` performs the explicitly requested update.
Both require the userdata profile, rollback mode `off`, a booted and recorded
unaccepted A trial, matching installed running core/modules, the retained B
image matching the protected baseline, and consistent A selection in GPT/UFS.
They refuse a stable, failed, identical or malformed target. They do not accept
the trial, enable automatic staging, change B's image or flags, change the UFS
selection, or request a reboot.

The writer quarantines A before replacing its image, verifies both complete
images, rearms only A's trial flags and records the new unaccepted candidate.
Original A bytes and checked metadata remain in RAM during the transaction;
no full image backup is created. On an ordinary failure it quarantines A again,
restores and verifies the old A image, then restores metadata and protected
state. If image restoration fails, it does not restore bootable metadata.
Shutdown/sleep inhibition and deferred termination signals protect the bounded
transaction, and existing battery, firmware, RPM and device checks still apply.

Only the existing narrowly validated matching-array backup-header CRC repair
is permitted. Other GPT damage, concurrent changes and ambiguous identities
remain refusals. Remote regular-file fixtures cover the real libfdisk
quarantine/rearm operations, transaction boundaries and partial image writes;
they cannot qualify real UFS, firmware retry or power-loss recovery.

A retained B image without matching installed core/modules is not a qualified
full rollback. This operation preserves that image and the existing manual
recovery route; it does not manufacture health approval or restore removed
packages. Keep the running unstable cohort installed until the next candidate
has completed physical validation. Compilation alone does not qualify camera
operation, fitted calibration, autofocus or promotion to the normal family.
