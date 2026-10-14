# Packaging admission

Add distribution-native rules only after the component's Uke source and payload
contract is reviewable. Do not publish empty runtime packages as hardware support.

## SSC startup candidate — 14 October 2026

`make srpm-ssc` produces pinned libssc and SSC-enabled SensorProxy sources.
`make srpm-ssc-fileservice` produces native FastRPC transport sources. These
targets require the retained upstream archives and Fedora RPM tools. Upstream
Meson runs on the build host only; runtime packages contain native code and
standard configuration. No KDE application is rebuilt.

The file-service units copy the sensor registry from read-only `persist` into
RAM. OEM configuration and `sns_tppe.so` must be installed separately from
verified device-local inputs. Binary firmware and calibration are not included
in these source packages. SensorProxy uses its standard desktop D-Bus interface;
sampling remains controlled by clients rather than permanent high-rate capture.

AArch64 package checks and disposable registry tests passed. Persistent tablet
startup, confined file-service access, ordering, restart and suspend tests remain
open. Do not install or enable these units as a qualified sensor stack yet.
Front light and magnetometer discovery remain unresolved; rear-light channels
have not been qualified as lux.

The 14 October candidates (`iio-sensor-proxy-3.9-4.uke3` and
`uke-ssc-fileservice-2026.09.21-2`) installed with their signed dependencies in
an isolated Fedora AArch64 Core root. RPM verification, dependency checking and
unit verification passed. This includes Fedora's system-bus provider; an earlier
minimal SDK exposed the missing `dbus.socket` dependency. No Python package was
requested by this transaction. These results do not qualify tablet startup.

`make srpm-sensor-tools` packages the existing C++ discovery and bounded-sample
clients as `uke-sensor-tools`. Its build runs the protocol/parser fixtures.
It installs two native commands, with no service, calibration, private input or
startup policy. Physical sensor availability and units remain separate checks.
