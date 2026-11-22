# Uke desktop integration

14 October 2026. `uke-adaptive-display` prepares the standard sensor interface
for automatic rotation and ambient brightness on Fedora Rawhide AArch64.
It uses SSC-enabled SensorProxy and original desktop applications. No KDE
application is rebuilt.

Activation is explicit. Device-local firmware and a read-only factory
calibration mount are required; neither is published in COPR. Physical axes,
front-light discovery, lux response and desktop behavior remain evening tests.
Read the [installation and test guide](docs/ADAPTIVE-DISPLAY.md) before enabling.

```sh
sudo dnf install uke-adaptive-display
```

Source and host checks are separate from tablet acceptance. The package does
not flash partitions or change refresh rates. RPM Version/Release fields use
numeric versions and release counters.

18 October 2026. Release 7 also requires the original Fedora ddcutil/libddcutil
packages and supplies PowerDevil defaults for display-only short power-button
presses. Existing per-user choices take precedence. Its additional udev rule
grants active-seat access only to I2C adapters below the upstream MSM DisplayPort
driver. Sensor/touch buses retain their existing permissions. External display
and DDC brightness acceptance still require a compatible connected monitor.
The package neither unmasks system sleep nor replaces KDE applications.
See [display controls](docs/DISPLAY-CONTROLS.md).

Release8 also prepares a PowerDevil-only libddcutil search path that excludes
actual internal GENI adapters while retaining DP AUX. The helper only reads
sysfs topology and writes private runtime configuration; existing user/admin
ddcutil files retain precedence. Offline topology and configuration tests pass;
package and tablet startup checks remain separate gates.

## Optional graphics source probes

29 October 2026. The explicit `vulkan-probe`, `gles-probe` and `opencl-probe`
Make targets retain the native diagnostic sources and require their respective
SDK headers and libraries. They are outside default package builds. Browser
workload fixtures and pinned research manifests are source tools; their
presence does not establish conformance or a new tablet result.
