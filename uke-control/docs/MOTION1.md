# Leased display inclination

Status, 14 November 2026: Control18 passes complete ARM source, native and signed
RPM qualification, with a tested DNF upgrade and rollback. Actual activation
fails at SELinux execution of its additional entry point. Candidate19 adds the
standard Fedora daemon binding and linked-policy permission checks; its runtime
and watchdog acceptance are pending. No normal Motion1 update is delivered.
Production IIO admission is explicitly false. Kernel26 does not admit the fitted gravity frame or acquisition-clock
mapping; an orientation label, rigid-body identifier, arrival timestamp or
another sensor's matrix cannot substitute for that evidence.

The numeric provider belongs inside the existing Control RPM. It is necessary
because original SensorProxy exposes categorical orientation rather than the
fresh screen-normal angle needed by the keyboard’s 45/40-degree policy. Hardware
protocols, topology and timestamp semantics remain in the kernel and Device Tree.
The existing SensorProxy integration, original Fedora SELinux policy and KDE
packages are retained. On the qualification tablet, the SensorProxy service and
executable belong to uke-iio-sensor-proxy 3.9-24; Motion1 does not migrate or
replace that provider.
There is no additional hardware-bridge runtime RPM.

## Session interface

The system-bus name and interface are `org.senemos.Uke.Motion1`; the object path
is `/org/senemos/Uke/Motion1`.

| Member | Input | Output |
| --- | --- | --- |
| `Claim` | none | known boolean, inclination double, sample BOOTTIME microseconds uint64 |
| `Release` | none | none |
| `PostureChanged` | directed to current claimers | the same boolean/double/uint64 tuple |

Inclination is 0 degrees when the outward display normal faces up, 90 degrees when vertical,
and 180 degrees when face down. The provider does not apply the keyboard's mode thresholds.
Unknown observations contain no usable angle; clients revoke rather than keep
a previous known value. Repeated claims/releases by one unique bus peer are
idempotent. Caller disappearance, inactive sessions and suspend revoke demand.

Authorization uses the unique system-bus peer, asynchronous Polkit checks and
active-local-session validation. A claim does not grant raw input or device
access. Signals are directed to lease holders. No entered text, clipboard data
or calibration is collected by this interface.

## Acquisition and cleanup

Only an admitted gravity endpoint and its own standard mount transform may
open acquisition. Scan layout, scale, timestamps, vector validity and generation
are checked before publishing an angle. The source rejects samples older than
1.5 seconds; it cannot refresh an old sample by changing its arrival timestamp.
The keyboard independently validates delivery age and applies 45/40-degree hysteresis.

A future admitted lease configures an exclusively owned buffer enable-last,
journals reversible settings, and restores them on release or recovery. Closing
an IIO file alone is not treated as disabling its buffer. The last lease removes
sensor demand; the broker has bounded idle exit. Healthy idle operation does not
poll hardware. The systemd unit requires an event-loop watchdog; the existing
frozen-loop fixture simulates supervisor termination and does not establish
actual systemd or tablet behavior.

## Original SELinux policy and package ownership

The helper uses the original `iiosensorproxy_t` domain with an explicit mandatory
unit context and empty capability set. Its local optional module adds only its
own executable/runtime objects, session notifications, watchdog notification and
D-Bus communication. A disabled original sensor domain cannot fall back to init_t.
The policy helper never enables an original module. Original and owned disabled
preferences survive reinstall; unmarked modules or conflicting priorities are
refused. Private Fedora-policy-store fixtures qualify these source operations.
Actual RPM scriptlet ordering, unit lifecycle and DNF rollback remain open.

The next Control integration must derive from signed Control 2.17.0-16 (COPR 11321859), retaining
all 64 source inputs, the closed-cover health fix, paired TUI, existing payloads,
SELinux kernel integration and persistent preferences. No fixed Control 15 factory or
partial userspace rebuild may replace that parent. A complete ARM RPM build,
upgrade/rollback checks and actual numeric sensor admission are required before
keyboard delivery. The keyboard's source/native tests do not establish them.

If the original upstream service gains equivalent numeric freshness, ownership
and authenticated lease semantics, migrate the client to it. Retire this internal
helper only after measured parity, a supported DNF transition and rollback are
verified.
