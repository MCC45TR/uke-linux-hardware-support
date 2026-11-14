# UKE-CONTROL-MOTION-20261113: lease a proved IIO frame without polling

Date: 13 November 2026. Environment: Uke, Fedora46 AArch64,
Control2.17.0-15 and Linux7.3.3-24-unstable. Source preparation only; no native
compilation, buffer configuration or sensor sampling has run on the tablet.

The separate keyboard's numeric client passes42/42 tests on three Fedora ARM
targets at sourcedd22b9c8/COPR11318576. The signed49 candidate remains outside
normal updates and is not installed. Its45/40 policy cannot use SensorProxy's
coarse tilt strings. Existing Control diagnostics take finite snapshots; they
are not a leased production stream. Motion1 belongs inside Control, not another
runtime package. The kernel/DT owner retains the cumulative queue and frame,
timestamp and hardware-lifetime admission.

## Sources and bounded findings

- Uke native SSC source and shared handoff retain the pinned SNS proto revision
  `32955196`. Gravity has six scalar values (gravity and linear acceleration),
  but its display-relative transform is not admitted. RIGID_BODY19 is an integer
  array of physical-body associations; DISPLAY0 does not establish axes. Attr20
  is reserved. No PLACEMENT decoder or copied accelerometer matrix is justified.
  No protocol code or fitted calibration is copied into userspace.
- The pinned donor survey and hardware acquisition inventory identify libssc
  `54dd13e36d65c21263cf5b0f4a70c230a77b26bc`, Hexagon RPC
  `da7a3742ab8d5273a281b70ee11f16fb2cb6a690` and original SensorProxy as relevant
  comparisons. Their protocol/discovery work does not supply the required
  standard numeric D-Bus API. The new consumer uses kernel IIO instead of
  duplicating those hardware protocols or adding a userspace fork.
- Original [Linuxv6.17 IIO core](https://github.com/torvalds/linux/blob/v6.17/drivers/iio/industrialio-core.c),
  `iio_chrdev_open/release` (GPL-2.0-only, Jonathan Cameron and contributors),
  rejects another character-device opener with EBUSY. Closing releases that
  ownership without disabling buffer capture. Explicit scoped restore and
  crash cleanup are therefore necessary. This older stable ABI reference is
  not proof that the currently fitted source has a valid frame or sample age.
- The [standard buffer ABI](https://docs.kernel.org/iio/iio_devbuf.html) describes
  channel indices, storage types, alignment, watermark and enable-last ordering.
  The consumer must read those declarations rather than assume a packed
  three-number vector. The fitted six-channel scalar layout and timestamp need
  separate actual-source and native qualification.
- Original [libsystemd event watchdog](https://www.freedesktop.org/software/systemd/man/latest/sd_event_set_watchdog.html)
  runs only while its event loop makes progress. Together with a systemd
  watchdog it can bound a frozen broker that still owns its bus name. Its
  periodic health notification is not hardware polling; no lease must mean no
  sensor work or persistent health timer. Provider unit/policy admission is
  not established by reading this API.

## Implementation boundary

The MIT C++ model decodes bounded scalar IIO layouts, requires an
explicit orthonormal mount, preserves the signed face-up/face-down distinction,
and validates stream epoch, ordering and actual sample age. It updates the
latest snapshot on each valid sample but publishes only known/band changes.
Its new test source is uncompiled. Missing frame and invalid data remain unknown.

New source adds asynchronous non-interactive Polkit requests, per-unique-name
leases, targeted signals, cancellation, caller loss and suspend/session teardown.
An event-driven IIO consumer requires its own gravity mount, scalar scan metadata
and boottime timestamps. Enable-last configuration has a root-only restoration
journal; recovery requires the same sysfs inode/device and an exclusive matching
character descriptor. A foreign setting prevents all restoration writes. The
service requires a systemd watchdog and exits after its last lease/pending claim;
it has no sensor poll timer. The separate scalar, transaction and actual-broker
private-bus tests are uncompiled; native validation is the next gate.

Production admission is deliberately closed in source: neither installed24 nor
candidate25 proves the gravity frame and acquisition-time contract. A subsequent
kernel capability is not yet defined. Do not open a device, select another
sensor or infer admission from a version, sensor name or arrival timestamp.
Narrow SELinux integration, actual IIO/journal fixtures, watchdog supervision and
the complete16 recipe/delivery still remain. No raw access is granted to
ordinary users. Use the existing Control runtime and keep
its paired TUI and Core24 SELinux capability unchanged. Derive a unique16 from
accepted15 inputs; do not rerun the published fixed15 factory. Signed native15
SRPM SHA256 is `d6be728b7fb2bb52497dbd91470249ff276e857d2de60db2d5e8659ca52bb71a`;
both signatures use the trusted project fingerprint
`2204a661ea630a5303500085538797e72a3e4eca`. Its spec is byte-identical to the
published `5a0b9d5236a9d071095e77ce0dbe6cb29b80a7f8968b972e3295327637723efc`.
The earlier source-worker SRPM has digests but no signature and is not this
signed parent. One required source reference is retained; the duplicate was
removed after its exact hash was checked. The active
Control worktree and all previous payload/overlays/settings remain untouched.

## Remaining validation

Qualify scalar decoding, byte alignment, timestamps, owner loss, malformed
data, partial setup, crash cleanup, foreign configuration and authentication
through COPR. Verify the complete inherited source/payload/dependency and
service-state closure before any DNF delivery. The current gravity mount,
hardware-time conversion and physical45/40 behavior are not established. A
streaming-cache callback fix does not by itself prove those properties.

First focused COPR11318696 uses immutable sourcea6b4e604 and manifest8b79e705.
All15 asset hashes match. GCC16 compiled the service and scalar test with
`-Wall -Wextra -Werror`; compilation of the transaction test then failed on
misleading indentation. No tests ran and no receipt/runtime RPM was produced.
Braces correct the test without reducing warning flags or its assertions.
Additional source validates journal attribute values and tests the actual
non-interactive Polkit encoding, bounded asynchronous failures and cancellation.
Those updates require a new native run; the first job is not qualification.

Focused11318702/source6eefac90/manifest435886f5 passes all four fixture groups
in the Fedora45 x86 source worker with GCC16 and unchanged warning/assertion
flags. Its actual worker script matches the request. The result directory now
contains a spec and sources, as required by the official custom-source API;
the previous keyboard diagnostic's SRPM-only result was not accepted output.
An empty target list selects the project's default chroot: the56-second
Rawhide ARM job built only a noarch receipt, not the component's ARM objects.
The receipt has two trusted signatures, SHA256c676a7d9; it installs one text
file and has no scriptlets. Temporary producer3282524 is auto_rebuild=false
and remains necessary while this qualification work is active.

Read-only tablet scan metadata then corrected the linear-acceleration names
to `in_accel_linear_[xyz]`. All seven scan indices/types are observed, not
assumed; current scale is0.000001 and the inactive buffer uses realtime.
No sysfs configuration or character device was opened. A separate journal
file layer now supports actual private-file tests of atomic no-replace,
abandoned precommit cleanup, bounded data and mode/owner/hardlink/symlink
refusal. Those changes and the exact scan names await the next native run.

Focused11318757/source5322bce5/manifestc1e232f5 passes all five groups,
including actual private journal files. The worker script and19 inputs match;
the57-second ARM job is again only a noarch receipt. Component ARM compilation,
device IIO ownership and the provider's actual systemd supervision remain open.

SELinux integration is now prepared against original Fedora policy
`5df11316337c290e108c8e8df637c31e87494c08` (GPL-2.0):
`policy/modules/contrib/iiosensorproxy.te/fc` and the standard systemd/init
interfaces. The exact TE SHA256 is
`56d138efdd37b2e29ecae5776688c356658e37ba175ab58b31b4bed0cafc6d28`.
That original domain already reads IIO and manages sysfs. The new helper reuses
it with an explicit mandatory unit context, empty capability set and no new
raw-device/user grant or shared-sysfs relabel. A small separate local module
adds only its private runtime journal, session-status/watch access, watchdog
notification and Motion1 D-Bus communication. Original policy files, SensorProxy
and Core24 remain unchanged. A disabled/missing original domain must fail
activation; do not silently enable a user-disabled module or fall back to init_t.

The remaining userspace delta is the numeric, authenticated, demand-leased API
that original SensorProxy does not expose. Kernel/DT still owns hardware frame,
timestamp and resource admission. If an original upstream service gains the
required numeric semantics and lifecycle, port the client and retire this
internal helper through a parity-checked DNF transition. The new local policy
and a real sd-event heartbeat/frozen-loop fixture are not yet native-qualified;
the latter simulates supervisor termination and cannot establish systemd unit
or tablet acceptance. An authorization-setup exception also gains a regression
test to ensure one cancellation reply rather than a second method error.

Focused11318767/source502c7087/manifestd03b4118 verifies all22 inputs and
compiles the service plus the new local policy using Fedora's original policy
development package. Its new watchdog fixture then fails to compile because
`<cstdint>` was missing; no fixture or Clang/sanitizer run occurred in this
attempt. Add the explicit standard headers and rerun the same gates. This is
not a runtime policy load or device result.

## 14 November: resume from signed Control16 and optional policy fixture

The tablet now runs Control/TUI16 and kernel26. The integration parent changes
to signed Control16/11321859: SRPMf1ef6c120692a8146cc288df70640c05b2be62321b60061f55103732566fada2,
recipee370e5b0705f7d4f6cfbff34ecc915c3eeb3e6fd582405b0d2f9e238d452b3a9,
64 non-spec inputs. All63 inherited Control15 inputs remain identical; preserve
the additional closed-cover health source and paired TUI. Prepare17, without
editing the other workstream's boot/kernel sources or installing anything.

Native5/11318772 previously passed all six GCC and Clang sanitizer groups,
including the private bus, authorization, journal and simulated watchdog tests.
It did not link the new module against the complete original policy store.
The new policy makes sensor-domain grants optional while keeping its own exec
and private-runtime types defined. The mandatory unit context still prevents
fallback when the user disables original iiosensorproxy. A new private-root
fixture links the unchanged Fedora45.24 original policy with that domain enabled
and disabled, preserves all priority100 HLL bytes and disabled preferences, and
checks removal of only the new types. This is prepared source, not native or
systemd/device acceptance. Own-module-disabled upgrade and foreign ownership
lifecycle checks still need package integration; this fixture does not claim them.
Production motion admission remains false; kernel26's diagnostic counter does
not establish the gravity mounting frame or measured acquisition clock mapping.
