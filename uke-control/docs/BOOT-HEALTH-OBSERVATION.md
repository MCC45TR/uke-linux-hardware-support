# Unapproved boot health reports

Date: 8 November 2026. Source preparation only; native and device acceptance
are required before delivery.

Control retains the native Uke hardware interfaces and the original distribution
desktop/audio packages. Boot approval, A/B selection and recovery policy belong
to its existing boot updater. They do not belong in the hardware kernel.

An unapproved trial now uses the existing bounded health window in every
rollback mode: 30 continuous healthy seconds within 90 seconds. Off mode runs
this observation during the boot action, without approving the kernel, selecting
a slot or requesting a reboot. DNF staging and dry runs acquire no additional
observation delay. Permissive and enforced acceptance retain their existing
state/identity checks and fallback policy. A successful observation is not
approval, complete hardware qualification or a production-release decision.

The existing service emits one start and one result record, prefixed
`UKE_KERNEL_HEALTH`, with schema, verdict, kernel release, current rollback mode,
pending approval, observation-only status and a bounded, single-line reason.
Rejected windows retain the last failing health check instead of reporting
only a generic timeout. There is no additional service or idle polling loop.
The sampling window is the existing bounded boot qualification operation.

Read the current boot with:

```sh
journalctl -b -u uke-userdata-accept.service --grep=UKE_KERNEL_HEALTH
```

The `rollback-policy status` JSON reports health observation separately from
automatic acceptance and fallback. A policy change during observation cannot
give the off-mode observer write or reboot authority. The normal acceptance
path re-reads policy before propagating a health failure. Existing failed-release
history, bounded systemd retries and approved-baseline requirements remain.

Native tests exercise all three modes, observation/acceptance paths, success,
failure, policy changes and log integrity using the real health-window helper.
No test has a device writer, acceptance callback or reboot provider. Actual
enforcing failure/fallback and interruption tests require a qualified recovery
baseline and are separate from these source tests.
