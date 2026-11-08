# Critical boot-health fallback

Date: 8 November 2026. Source candidate; native and device admission pending.

The rejected full8 boots with a matching package but its ADSP crashes. Audio
requests time out and battery telemetry returns EPIPE. Checking only active
services and an enabled display incorrectly calls this healthy. The boot probe
now also observes the native remoteproc state without starting a DSP request.

Ordinary boot-health failures retain their previous behavior. A confirmed ADSP
crash is a separate critical failure: off logs it; permissive and enforced
terminate the acceptance attempt with the existing immediate-rejection status.
The existing failure unit performs fallback. No additional runtime RPM or
service is introduced. The existing acceptance unit also remains as a blocking event consumer after
boot qualification. It observes remoteproc/coredump uevents and atomic policy
and state-file changes, without a timer or repeated DSP queries. A late failure
uses the retained same-boot rollback pair, including after boot acceptance.
Replacement of that other slot or removal of its modules causes refusal.
The unchanged service name uses Type=exec so its event wait does not block the
graphical target. Native event and actual fallback qualification remain pending.

A root-owned rejection record binds the exact boot ID, kernel and SHA256 of
the protected slot state. Permissive fallback requires this current record.
Stale records cannot authorize selection. The prior slot image, installed
bundle/modules, firmware, GPT and UFS identities remain required. An unapproved
development checkpoint may be a critical-only permissive return target when
these existing validations pass; it does not become production approved.
In-place trial restrictions remain. Returning to the retained slot keeps the
existing failed-release bookkeeping and prevents automatic retry loops.

Image writes retain the measured-battery requirement. Only a critical fallback
metadata transaction may use the independent, positively online USB-C power
provider when a crashed DSP prevents battery telemetry. With neither valid
battery nor independent power it refuses and logs the reason. No battery
percentage, firmware state or hardware capability is fabricated.

Source fixtures check the actual sysfs probe, ordinary/critical distinction,
mode changes, one-boot authority and stale-record refusal. Native compilation,
real failed-trial return, interruption, power loss and runtime monitoring are
separate acceptance gates. The system keeps its user-selected permissive mode;
the distribution default remains enforced.

The crash event is latched for this boot before probing state, so a firmware recovery that finishes before userspace dispatch cannot hide the crash. Native tests also exercise the actual late-pair identity predicate and emergency metadata-power provider using temporary files. Device fallback and power-loss acceptance remain separate.

A critical event that overlaps another updater keeps one unheld lock descriptor and also observes the other writer's close. This covers read-only checks that release the lock without changing userdata.state. Its own descriptor remains open while waiting, preventing a self-generated close/retry loop; healthy idle does not observe lock-close traffic. Native and device scheduling acceptance remain separate.
