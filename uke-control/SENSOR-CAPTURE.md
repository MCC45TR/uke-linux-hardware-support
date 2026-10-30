# Native sensor capture

30 October 2026. Control 2.5.0 provides bounded native JSON Lines for motion,
front CCT streaming and IMU temperature. COPR and tablet results are recorded
separately. This interface does not replace original SensorProxy or KDE.

Use explicit acquisition from the main Control package. A collection keeps one
QRTR client, closes it on completion, and changes no calibration or registry:

```sh
sudo uke-control sensors capture sensor_temperature 10 --json
sudo uke-control sensors capture gyro 30 --json
sudo uke-control sensors capture cct_front_strm 10 --json
```

Durations are 1–60 seconds after the existing three-second motion stabilization
interval. Acceleration and gyro request 26 Hz, temperature
1 Hz and the OEM CCT stream 5 Hz. Admission requires the exact datatype/API,
availability and streaming attributes. Unexpected vectors, source changes,
non-finite values and timestamp discontinuities fail the observation.

Each sample carries raw DSP ticks and a BOOTTIME receipt timestamp. The final
summary reports observed receipt rate, DSP tick intervals, means and standard
deviations. Gyro angular integration estimates its clock from the receipt
interval; it is not an independently synchronized clock or calibrated drift
measurement. Motion, batching and delivery jitter affect the estimate. A
stationary physical reference is required before interpreting a mean as bias.
Unreliable samples prevent angular integration from being reported.

The temperature API defines one physical sensor value in Celsius. It measures
the IMU, not the room, battery or SoC. Firmware calibration is not independently
certified by the wire accuracy flag. CCT data remains explicitly unverified in
units; zero, a live publisher or a new event cannot authorize automatic white
point adjustment. The persistent on-change publisher retains honest freshness.

`desktop status --json` reads logind cover state and original KWin tablet mode.
PowerDevil owns cover actions; KWin owns rotation and saved output layouts.
This package adds no cover handler or forced layout. `charge protection-status`
adds `moisture_state`: stale/disabled telemetry remains unknown, positive wet
reports remain visible, and `dry_reported` does not certify electrical protection.
No detector enable request or protection bypass is issued by status queries.
