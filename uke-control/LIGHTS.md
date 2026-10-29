# Uke light controls

25 October 2026. Control 1.12.0 uses the original kernel LED providers on
Xiaomi Pad 7 / POCO Pad X1. RGB policy now belongs to the kernel; Control is a
request broker and client. Host source checks do not establish physical light
output, electrical measurements or acceptance of a new kernel/RPM on the tablet.

```sh
uke-control flashlight status
sudo uke-control flashlight set 50
sudo uke-control flashlight set 100
sudo uke-control flashlight off
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

The broker requires SELinux Enforcing, its exact domain and admitted device
ownership/type before acquiring the bus name. The service has no capabilities,
uses device cgroup restrictions and cannot write sysfs, GPIO, firmware or
regulators. It transports ioctls; it contains no animation loop or battery
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

Torch brightness still accepts 0–100. The retained **20,000 microampere total**
ceiling and 5 mA per-channel resolution provide two nonzero steps: 50% requests
10 mA total and 100% requests 20 mA total. Zero turns it off. Positive
intermediate requests round down to the minimum nonzero step. `on` aliases
100%. The reviewed provider has `max_brightness=2`; older 255-scale providers
are refused.

The torch worker has no fixed deadline. Manual off, normal stop, signals and
failure cleanup request zero; it stops before system sleep, has no restart
loop and saves no persistent on policy. Its original QCOM flash driver,
PMXR2230 rear channels 1 and 4 and 20 mA total ceiling are unchanged. No strobe,
regulator voltage or arbitrary sysfs path is exposed. Removing the deadline
does not establish continuous-use thermal acceptance.

Remaining physical validation includes visible color/order/timing, measured
electrical/thermal behavior, device/domain permissions under SELinux Enforcing,
service/driver removal and suspend/resume. Installing an RPM does not activate
its DT graph; the matched kernel must be booted separately.
