# uke-platform-runtime

Provider correction, **18 October 2026**: installed release2 failed discovery
because the auxiliary bus uses the qualified driver name
`qcom_battmgr.pmic_glink_power_supply`. Release3 source shares an exact canonical
auxiliary-driver resolver and corrected positive/negative fixtures; these changes
have not been built/tested here. Earlier host/package passes do not establish
installed provider or charging acceptance. See the
[observation contract](docs/BATTERY.md) and [control contract](docs/CHARGE-CONTROL.md).

Status, **18 October 2026**: the earlier native read-only battery observer for Xiaomi
Pad 7 / POCO Pad X1 (`uke`, SM7675) compiles in a Fedora Rawhide AArch64 SDK.
Model, session, profile, synthetic collector and private-bus API tests pass.
COPR builds 11166039 (UPower) and 11166040 (observer) and direct payload audits
pass at their recorded pins. The new charge-control source below has not been
built or tested. Target dependency admission and tablet service tests remain pending. This is not
physical charging acceptance. The optional original UPower overlay has
separate [host results and installation limits](packaging/upower/README.md).

The C++ observer reads an audited standard power-supply allowlist and offers
read-only JSON and D-Bus snapshots. Missing/stale data stay unavailable. Health,
EFC and duration models share one session across CLI/daemon observations; unqualified
firmware/pack data do not produce precise health or time claims. New charging
controls, thresholds, high power, PPS and EPR remain disabled.
The model identity resolver requires privileged, matching per-boot evidence with
an expiry and never enables an actuator. See
[runtime identity and recovery](docs/RUNTIME-PROFILE.md).

A separate [charge-control source candidate](docs/CHARGE-CONTROL.md) implements
guarded standard Linux thresholds for original KDE, positive OEM FCC writes,
slow mode and automatic FCC adaptation with firmware-owned CV. Its kernel
options, services and shipped profile remain off/unqualified. Exact firmware,
pack, source, protections, measurements and readback admission are required;
no physical control or hardware safety is qualified by these source changes.
The cache-only `protocols` report separates manufacturer receiver declarations
from unknown negotiated sessions; original USB wire6, OEM adapter7 and vendor XM
spaces are not interchangeable. Control qualification also requires reviewed
protocol-ABI, source-contract and cable-contract proof hashes.

[Battery contract and limitations](docs/BATTERY.md),
[qualification record](manifests/battery-profile.json),
[readiness](manifests/battery-readiness.json).

Qualcomm remoteproc, QRTR, FastRPC and SSC integration remains a separate scope.
This component does not start another remote processor or claim completed
hexagonrpc/libssc payloads. Use the original Fedora KDE applications.

Host entry points are `make all`, `make test-native` and `make srpm`.
The RPM source produces separate `uke-battery-observer` and `uke-charge-control`
packages for Fedora AArch64. The observer remains read-only. Other
runtime package names and distribution targets are not implemented by this
recipe. Python does not enter its target payload. Archives stay in
`referances/`; original implementation lives in `src/`.
