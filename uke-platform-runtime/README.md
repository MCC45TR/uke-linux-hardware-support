# uke-platform-runtime

Status, **18 October 2026**: the native read-only battery observer for Xiaomi
Pad 7 / POCO Pad X1 (`uke`, SM7675) compiles in a Fedora Rawhide AArch64 SDK.
Model, session, profile, synthetic collector and private-bus API tests pass.
RPM/dependency admission and tablet service tests remain pending. This is not
physical charging acceptance. The optional original UPower overlay has
separate [host results and installation limits](packaging/upower/README.md).

The C++ observer reads an audited standard power-supply allowlist and offers
read-only JSON and D-Bus snapshots. Missing/stale data stay unavailable. Health,
EFC and duration models share one session across CLI/daemon observations; unqualified
firmware/pack data do not produce precise health or time claims. New charging
controls, thresholds, high power, PPS and EPR remain disabled.
The identity resolver requires privileged, matching per-boot evidence with an
expiry. The installed profile is unqualified; no evidence record enables a
charging actuator. See [runtime identity and recovery](docs/RUNTIME-PROFILE.md).

[Battery contract and limitations](docs/BATTERY.md),
[qualification record](manifests/battery-profile.json),
[readiness](manifests/battery-readiness.json).

Qualcomm remoteproc, QRTR, FastRPC and SSC integration remains a separate scope.
This component does not start another remote processor or claim completed
hexagonrpc/libssc payloads. Use the original Fedora KDE applications.

Host entry points are `make all`, `make test-native` and `make srpm`.
The first RPM candidate is `uke-battery-observer` for Fedora AArch64. Other
runtime package names and distribution targets are not implemented by this
recipe. Python does not enter its target payload. Archives stay in
`referances/`; original implementation lives in `src/`.
