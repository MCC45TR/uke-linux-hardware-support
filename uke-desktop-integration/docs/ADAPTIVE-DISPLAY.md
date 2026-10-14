# Automatic rotation and ambient brightness preview

14 October 2026. Target: Fedora Rawhide AArch64 on Xiaomi Pad 7 / POCO Pad X1
(`uke`, SM7675). Package: `uke-adaptive-display`.

The package selects SSC-enabled `iio-sensor-proxy`, libssc and the native
FastRPC file service. It provides the standard `net.hadess.SensorProxy` D-Bus
interface used by original KDE and other compatible desktop environments.
No KDE application, private firmware or calibration data is included.

The current KWin sources consume accelerometer orientation and ambient light.
Automatic brightness requires a valid light reading in **lux**, a supported
brightness device and the corresponding output capability. Raw SIP1328 channel
values are not a substitute. The front-light path and measured axes remain
physical test gates; installing this package cannot repair missing sensor data.

## Before activation

Use the matched Uke kernel/DT and admitted device-local sensor files. The file
service requires these paths from a separately installed local data package:

- `/usr/share/uke-sensors/odm/json.lst`
- `/usr/share/uke-sensors/sns_reg.conf`
- `/usr/lib/firmware/qcom/uke/sensors/sns_tppe.so`

The existing `/mnt/vendor/persist` mount must be read-only. The root-stage
service refuses startup if it is not a read-only mount. Registry and firmware
inputs are copied to private RAM storage below `/run/uke-ssc`; the original
calibration partition is never made writable. Do not replace absent inputs
with files from another device or bypass DNF requirements.

## Install for the evening trial

```sh
sudo dnf copr enable mcc45tr/uke-linux-test
sudo dnf install uke-adaptive-display
findmnt --mountpoint /mnt/vendor/persist --output TARGET,OPTIONS
sudo systemctl enable --now uke-adaptive-display.target
systemctl --no-pager status uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service
```

Installation leaves the integration target disabled. Enable it only after the
local-input and read-only-mount checks pass. It affects sensors only; it does
not flash boot partitions, change display modes or replace display settings.

Inspect the standard interface before changing the desktop:

```sh
busctl get-property net.hadess.SensorProxy /net/hadess/SensorProxy net.hadess.SensorProxy HasAccelerometer
busctl get-property net.hadess.SensorProxy /net/hadess/SensorProxy net.hadess.SensorProxy HasAmbientLight
busctl get-property net.hadess.SensorProxy /net/hadess/SensorProxy net.hadess.SensorProxy LightLevelUnit
monitor-sensor --accel
monitor-sensor --light
```

Stop each monitor with Ctrl+C. In the original KDE Display Configuration page,
select automatic rotation for the internal panel. Test all four physical poses,
including touch alignment. Enable automatic brightness only when the frontend
reports an ambient sensor in lux and KWin exposes that capability. Compare a
covered sensor and two lighting levels, check settling/flicker, then verify
manual override and display off/on. Keep the previous manual brightness until
those checks pass. Older desktops may not implement automatic brightness even
when SensorProxy is available.

Do not change refresh rates during this first sensor trial. Cold boot, service
restart, suspend/resume, four-pose axes and calibrated light response remain
separate acceptance tests.

## Stop the preview

```sh
sudo systemctl disable --now uke-adaptive-display.target
sudo systemctl stop iio-sensor-proxy.service uke-ssc-sensors.service uke-ssc-root.service
```

Restore manual rotation/brightness in the desktop if selected. Removing the
integration package does not delete device-local firmware or factory calibration.

## Source references

- [KWin orientation client](https://invent.kde.org/plasma/kwin/-/blob/master/src/utils/orientationsensor.cpp)
- [KWin light client](https://invent.kde.org/plasma/kwin/-/blob/master/src/utils/lightsensor.cpp)
- [KWin output capabilities](https://invent.kde.org/plasma/kwin/-/blob/master/src/backends/drm/drm_output.cpp)
- [libssc](https://codeberg.org/DylanVanAssche/libssc)
- [SensorProxy](https://gitlab.freedesktop.org/hadess/iio-sensor-proxy)

Upstream sources were inspected through their public source APIs. Host and
package results do not establish physical sensor, display or calibration support.
