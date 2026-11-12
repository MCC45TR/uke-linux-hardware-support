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
