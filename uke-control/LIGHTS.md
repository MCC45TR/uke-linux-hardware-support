# Uke light controls

25 October 2026. Control 1.11.0 uses original kernel LED class interfaces on
Xiaomi Pad 7 / POCO Pad X1. This source revision has no new kernel/RPM build or
tablet result. Host checks do not establish physical light output or measured
current/voltage.

```sh
uke-control flashlight status
sudo uke-control flashlight set 50
sudo uke-control flashlight set 100
sudo uke-control flashlight off
uke-control led status
sudo uke-control led set 255 0 128 50
sudo uke-control led off
```

Torch brightness accepts 0 through 100. The retained **20,000 microampere total**
ceiling and PMIC's 5 mA per-channel resolution provide two nonzero steps:
50% requests 10 mA total, 100% requests 20 mA total. Zero turns it off. Positive
intermediate requests round down to the minimum nonzero step; status reports
the applied step, not a fabricated continuous scale. `on` is an alias for 100%.
The reviewed kernel exposes `max_brightness=2`; older 255-scale providers are
refused, even if their label matches.

The worker has **no fixed automatic deadline**. A root-only runtime target lets
it apply changes while active; it owns the actual writes and readback. Manual
off, normal stop, signals and service failure cleanup request zero brightness.
The service conflicts with and stops before `sleep.target`, is never enabled at
boot, has no restart loop and saves no persistent on policy. An update does not
restart the worker. Removing the deadline does not establish continuous-use
thermal acceptance.

The exact reviewed kernel graph is required: original QCOM flash driver,
PMXR2230 rear channels 1 and 4, and the 20 mA total torch ceiling. This is 10 mA
per channel at 100%, below the retained OEM 250 mA per-channel limit. No
flash/strobe mode, regulator voltage control or arbitrary sysfs path is exposed.
The original driver's protection and current clamp remain active.

RGB accepts R/G/B components 0 through 255 and an optional brightness percentage
0 through 100 (default 100 for the original three-component command). Color and
brightness are independent. **100% means PWM brightness 8 out of 511**, preserving
the previous low-duty ceiling; this is not full electrical power or an ampere
measurement. The PWM scale provides eight nonzero levels. Positive intermediate
percentages round down, with a minimum of one; status reports the applied level.
Color changes first turn the output off, set every intensity in the reported
channel order, then apply the bounded brightness. Failures attempt to leave it
off. No notification or camera automation is installed.

The widget provides a torch brightness dialog and RGB components/brightness
with a color preview. Apply sends one validated request through `uke-control`;
slider movement itself performs no actuation or authorization request.

Missing or mismatched board identity, provider, driver, current limit, channel
mapping, scale or trigger ownership prevents changes. Provider identity follows
the parent controller's DT link and exact LED label. Installing a kernel RPM
alone does not activate its DT graph; a matched kernel must be booted separately.

Physical acceptance remains open: actual PMIC probe, visible output, color
mapping, measured electrical/thermal behavior, stop/failure and suspend
lifecycle. A status response or source check is not these results.
