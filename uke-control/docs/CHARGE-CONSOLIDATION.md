# Charge helper ownership in Control

11 November 2026. Control217 folds the existing charge helper, three units,
udev rule, disabled presets, readiness metadata and two configuration files
into the existing Control runtime. The two other cohort names remain the TUI
and client development package. No runtime package name or service is added.
The old charge package is provided and obsoleted at1.4.1-8; the existing Qt
frontend's dependency remains satisfiable. Battery observer and offline charger
outputs still have separate roles and are not retired by this change.

Apply only the native91 charge bridge delta to the actual Control216r2 source.
Do not overwrite its source with a checkout: the packaged main/CLI additionally
contain rollback actions absent from the source-check checkout. Pin actual
preimages and whole-file inverse checks so those actions and all other current
features survive. The complete new helper retains the qualified native91
single-write errno/short-write behavior and original zero-argument library ABI.
An opaque firmware receipt does not establish electrical cutoff.

The three unit definitions, default configuration, readiness data, rule and
presets retain the actual old package bytes. Both configuration files retain
RPM config(noreplace) ownership. The pre/post-transaction fragments snapshot
bounded unit state before old package removal and restore active units and
enabled/runtime/masked preferences afterwards. They reject stale, malformed,
foreign, linked and transitional state. A newly applied mask takes precedence.
Fresh installs do not create a migration record or start a new hardware path.
The retained protection service can still be activated by the existing udev
coldplug rule; this is not a new worker or qualified autonomous charge control.

The current protection observer still uses the legacy two-second observation
loop. Moving fault/lifecycle coverage to native kernel events remains open;
this ownership fold must not be described as completing that migration. Keep
firmware protocols, resource/fault ownership and mandatory hardware limits in
kernel/DT. Keep necessary demand/session integration within Control and history,
estimates and policy in original distribution services when parity is proven.

Before installation, require the native package build, retained ABI/TUI/source
gates, unit-state fixtures, signed file/provider/dependency audit, actual DNF
upgrade/reinstall/fresh/rollback previews and configuration preservation. An
applied transaction, active service state, physical charging behavior and
whole-system protection acceptance are distinct gates. Do not retire the
remaining battery producer or original-provider fork prematurely.
