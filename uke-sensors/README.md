# Uke sensors

14 October 2026. Preview RPMs provide libssc, SSC-enabled SensorProxy and a
native FastRPC file service for Fedora Rawhide AArch64. The standard desktop
interface supplies orientation and ambient-light data when the device firmware
exposes those sensors. Original KDE applications are used.

A stationary orientation event was observed in the earlier bounded tablet
trial. Automatic four-pose rotation, front-light discovery, light units and
persistent lifecycle remain separate tests. Raw rear-light channels are not
reported as calibrated lux. Package builds do not establish sensor support.

The `uke-adaptive-display` package selects this stack for an explicit evening
trial. [Read the activation and test guide](../uke-desktop-integration/docs/ADAPTIVE-DISPLAY.md).
Device-local firmware and read-only factory calibration are required separately;
COPR contains neither. Registry updates stay in private RAM storage.

[Packaging and host dependencies](packaging/README.md) ·
[Source pins](manifests/ssc-desktop-source.json)
