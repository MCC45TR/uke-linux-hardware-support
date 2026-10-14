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
not flash partitions, change refresh rates or claim support for other desktop
policies. RPM Version/Release fields use numeric versions and release counters.
