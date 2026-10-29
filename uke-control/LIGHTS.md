# Uke light controls

25 October 2026. Control 1.13.0 uses the original kernel LED providers on
Xiaomi Pad 7 / POCO Pad X1. RGB policy now belongs to the kernel; Control is a
request broker and client. Host source checks do not establish physical light
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
requests from an active local session through Polkit without authentication
interaction. Inactive, remote and root-origin writes are refused by the API.
Normal users receive no direct device or LED sysfs write permission. An
administrator who changes the security policy remains outside this boundary.

The brokers require SELinux Enforcing, their exact domain and admitted device
ownership/type before acquiring the bus name. The service has no capabilities,
uses device cgroup restrictions. RGB's unit has read-only sysfs; the torch unit
admits only its brightness attribute. Neither exposes GPIO, firmware or
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

The torch slider accepts **1–100** without root, sudo or pkexec. The retained **20,000 microampere total**
ceiling and 5 mA per-channel resolution provide two nonzero steps: 50% requests
10 mA total and 100% requests 20 mA total. Zero turns it off. Positive
intermediate requests round down to the minimum nonzero step. CLI zero is a
compatibility alias for off; `on` restores the last request in this service
lifetime (default 100). Status and the widget distinguish the requested
percentage from the applied 50/100 step. The reviewed provider has
`max_brightness=2`; older 255-scale providers
are refused.

Torch has its own fixed D-Bus interface and active-local-only Polkit action.
A native package/udev helper validates the exact existing board, driver, DT
mapping, scale and current limit before changing metadata on that one sysfs
brightness file. It grants root:`uke-rgb` mode 0664 and SELinux type
`uke_torch_brightness_t`; it never writes a light value. No desktop group
membership, setuid executable or arbitrary path request is installed. Final
package removal restores root:root mode 0644 and the generic sysfs label.
The non-root actuator retains a descriptor and checks provider/readback on
each request. Its unit has no capabilities and stops independently of RGB.

The torch worker has no fixed deadline. Manual off, normal stop, signals and
failure cleanup request zero; it stops before system sleep, has no restart
loop and saves no persistent on policy. Its original QCOM flash driver,
PMXR2230 rear channels 1 and 4 and 20 mA total ceiling are unchanged. No strobe,
regulator voltage or arbitrary sysfs path is exposed. Removing the deadline
does not establish continuous-use thermal acceptance.

Remaining physical validation includes calibrated colors and waveform timing,
measured electrical/thermal behavior, actual battery thresholds, camera
integration, service/driver removal and suspend/resume. Installing an RPM does
not activate its DT graph; the matched kernel must be booted separately.
