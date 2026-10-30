# Uke light controls

30 October 2026. Control 2.4.0 uses the original kernel LED providers on
Xiaomi Pad 7 / POCO Pad X1. RGB and torch policy belong to the kernel; Control is a
request broker and client. See [the ownership contract](OWNERSHIP.md). Host source checks do not establish physical light
output, electrical measurements or acceptance of a new kernel/RPM on the tablet.

On POCO Pad X1, signed Control/core/tests 1.12.0-5 from
[COPR 11205434](https://copr.fedorainfracloud.org/coprs/build/11205434) and Linux
7.3.0-5 pass the bounded visible color-sweep trial. The operator confirms the
transition; readback returns to off/zero with no kernel I/O error. Enforcing,
the actual confined process, NNP and caller/device permission checks pass.
This result does not establish calibrated colors, electrical/thermal endurance,
battery warnings, camera behavior, suspend or supplier-removal acceptance.

```sh
uke-control flashlight status
uke-control flashlight set 50
uke-control flashlight set 100
uke-control flashlight off
uke-control led status
uke-control led set '#ffff00' 50
uke-control led test
uke-control led auto
```

RGB requests use no sudo or pkexec. The dedicated `uke-rgb` service account is
locked and owns no login session. `/dev/uke-rgb` is root:`uke-rgb`, mode 0660,
with SELinux type `uke_rgb_device_t`. Only the confined `uke_rgb_t` broker gets
the device permission in the supplied policy. Its fixed D-Bus methods accept
requests from an active local session through the original system authentication
window. A fixed two-minute grant belongs only to that D-Bus connection; it is
not shared with another application under the same UID. Inactive, remote and root-origin writes are refused by the API.
Normal users receive no direct device or LED sysfs write permission. An
administrator who changes the security policy remains outside this boundary.

The brokers require SELinux Enforcing, their exact domain and admitted device
ownership/type before acquiring the bus name. The service has no capabilities,
uses device cgroup restrictions. Both units have read-only sysfs and admit only
their matching character device. Neither exposes GPIO, firmware or
regulators. RGB transports ioctls; it contains no animation loop or battery
monitor. Package installation loads the policy before triggering device
activation. A missing or mismatched kernel device leaves RGB unavailable.

| Kernel policy | Behavior |
|---|---|
| Capacity below 15%, at least 5% | Red breathes for 3 seconds, then stays off for 2 seconds; 5-second loop |
| Capacity below 5% | Red on for 1 second, off for 2 seconds; no fade |
| Future front-camera stream | Green breathing in 2-second loops; kernel hook only, no camera integration yet |
| Explicit color test | One 10-second color-wheel sweep, including yellow and intermediate colors |
| Manual HEX request | `#RRGGBB` and brightness 0–100; the legacy three-component CLI remains accepted |

Priority is camera, explicit bounded test, battery warning, manual color, off.
The kernel reads the exact battery supply's reported capacity; unavailable
telemetry preserves an existing warning. `auto` and the compatibility alias
`off` clear manual/test state and retain automatic warnings. Only one writer
may open the device. Closing it restores automatic policy, including after a
service failure. Kernel ownership blocks competing brightness, intensity and
trigger stores. Full suspend/shutdown request off; resume refreshes capacity.
No unreviewed LPG SDAM/PBS pattern is enabled, so animation during full suspend
is not implemented.

RGB 100% means native PWM brightness **8/511**, retaining the existing low-duty
ceiling. There are eight nonzero brightness levels; positive percentages round
down with a minimum of one. Color mixing and breathing inherit that resolution.
HEX denotes requested channel ratios, not a calibrated visible color or an
ampere measurement. The original QCOM LPG driver retains electrical control;
no voltage, peak-current or regulator setting is increased. Status reports
kernel state and an I/O error separately; `physical_acceptance` stays null.

The widget keeps only the **Test colors** RGB button. It sends one fixed
`uke-control led test` request and displays the kernel's remaining time.
Repeating a running test is refused; it does not extend the deadline. Widget
testing uses a separate original Fedora `plasmoidviewer` without restarting
Plasma.

The torch request range is **1–100** without running the client as root, sudo or
pkexec. Changes require the original system authorization window; a persistent
client connection reuses its own fixed two-minute grant. The retained **20,000 microampere total**
ceiling and 5 mA per-channel resolution provide two nonzero steps: 50% requests
10 mA total and 100% requests 20 mA total. Zero turns it off. Positive
intermediate requests round down to the minimum nonzero step. CLI zero is a
compatibility alias for off; `on` restores the last request in this service
lifetime (default 100). Status and the widget distinguish the requested
percentage from the applied 50/100 step. The reviewed provider has
`max_brightness=2`; older 255-scale providers
are refused.

Torch has its own fixed D-Bus interface and active-local-only Polkit action.
The kernel validates the exact board, driver, DT mapping, scale and current limit
before exposing `/dev/uke-torch`. Udev grants root:`uke-rgb` mode 0660 and
SELinux type `uke_torch_device_t`; normal users have no direct access. The
broker sends fixed ioctls and reports kernel status, including unknown brightness
and the original I/O error when output cannot be confirmed by the provider.
The kernel owns the two current steps, remembered positive request, exclusive
writer and off on writer close, system sleep, shutdown and provider removal.
Closing a read-only status descriptor leaves the writer unchanged. Resume never
automatically restores an on request.

There is no fixed deadline or persistent-on policy. The original QCOM flash
driver, PMXR2230 rear channels 1 and 4 and 20 mA total ceiling remain unchanged.
No strobe, regulator voltage or arbitrary sysfs path is exposed. The previous
sysfs permission helper and userspace cleanup are removed; the RPM upgrade
revokes the old grant before replacing its policy. Kernels without the new
character-device ABI leave torch requests unavailable. Source and fixture checks
do not establish continuous-use thermal acceptance.

Remaining physical validation includes calibrated colors and waveform timing,
measured electrical/thermal behavior, actual battery thresholds, camera
integration, service/driver removal and suspend/resume. Installing an RPM does
not activate its DT graph; the matched kernel must be booted separately.
