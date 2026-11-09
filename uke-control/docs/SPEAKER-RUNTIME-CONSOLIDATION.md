# Speaker runtime ownership in Control

This candidate folds the existing guarded speaker helper into `uke-control`.
The kernel retains amplifier/DSP topology, gain limits, mute-on-fault and stream
lifecycle. The nonresident userspace helper still reads device calibration from
read-only persist and stages it through the authenticated ALSA interface. Private
calibration and generic firmware acquisition do not belong in Device Tree.

The original Fedora PipeWire and WirePlumber packages remain the audio session
providers. The mandatory matching Control TUI, existing updater, A/B recovery,
brightness request and all previous Control inputs remain in the same cohort.
There is no additional runtime RPM, daemon, sensor polling or boot network call.

## Compatibility and provisioning

The five existing executable, service and udev paths are retained. Compatibility
Provides cover the old name, ISA and gain-readiness capability; the preflight
queries the capability instead of requiring a literal old RPM name. Existing
kernel dependencies continue to resolve. Gain223 and every gain243 physical
qualification refusal remain unchanged.

The sound-device event still starts the same oneshot template. Its new
`--ensure-loaded` operation authenticates and locks the real kernel command
numid, then reads protection state. Ready, prepared and running return without
opening private/generic sources or issuing a DSP write. Empty retains the
original guarded provisioning. Staging, fault and unknown refuse; this operation
does not clear a fault or restart ADSP. Explicit `--load` remains available.

The old package removal script disables the template with `--now`. The new RPM
therefore snapshots only the matching loaded instances before the ownership
transition and restores previously active instances after removal. Inactive and
masked paths are not started; masks and enabled/runtime states are preserved.
Transitional or active-but-masked state refuses before migration. The bounded
private record rejects stale, malformed, duplicate, foreign, linked or permissive
data before systemd changes. Restore uses the idempotent native operation and
does not issue a global sound-device trigger. A failed transaction record is
retained for diagnosis; it is never silently overwritten on retry.

## Source and licensing

- Control parent: source `580fdf004ca35ee00f7d5cac8c3590f9d986beb9`,
  COPR11291213, SRPM SHA256
  `432db4bc4e40eda63700af4ed0b9d22ad6555a3a7c35d566f107dc3abae23c43`.
- Original project speaker runtime: source
  `63cdc4c65fbf50f6f5b972bc538e8fcc38b3922d`, COPR11264671,
  SRPM SHA256
  `22f97cc686e6746cb035865fbf9ba6e7161d3ba00034f8dc7742dd942db286c4`.
  Its archive, file-level GPL-2.0-only notices and original MIT license notice
  remain available. The small patch changes only the idempotent entry point,
  provider query, template argument and host-only mocks.
- GPL text: Linux `d50d994bffa3c7c6db2d8bc4c5f0c55e3f46b0cd`,
  `LICENSES/preferred/GPL-2.0`, SHA256
  `8780e78a1a737e127f25a65f6d95269bffd36158dc261114de7859b490bfc5aa`.
  No kernel code, board values, firmware or calibration are copied by this fold.

## Acceptance boundaries

COPR must run the original runtime/service/preflight contracts, four native
compiler/sanitizer modes for the real idempotent helper, provider compatibility
and the actual expanded RPM migration fragments with fake systemd. Those are
source/package evidence, not physical audio or amplifier-protection acceptance.

Retiring the old COPR producer also requires signed RPM/ownership/dependency
admission, actual DNF upgrade/fresh/downgrade checks and recoverable device tests
for speakers, microphone, DP, STOP/reopen and suspend. Preserve the old provider
and accepted kernel until this measured parity and rollback path are recorded.
