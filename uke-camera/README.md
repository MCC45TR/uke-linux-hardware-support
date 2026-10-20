# uke-camera

Uke camera source, media graph and userspace pipeline bring-up for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

20 October 2026: an [inactive OV13B10 kernel prerequisite](https://github.com/MCC45TR/senemos-uke-kernel-mainline/commit/5c64172)
adds generic Device Tree matching and a strict binding. The complete driver
object compiles for AArch64; real schema tests accept two/four lanes and reject
eight invalid cases. No board sensor node, CAMSS platform, firmware startup or
physical capture is enabled. The fitted rear module and front sensor identity,
native provider/CSI graph and calibration remain unresolved.

`uke-control hardware status` reads existing media endpoints and configured
camera/audio nodes without opening a camera or triggering a probe. The
[path review](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/testing/UKE-NATIVE-HARDWARE-PATHS-2026-10-20.md)
records the source pins and remaining work; documentation access is required.

Status, 13 October 2026: this component contains package definitions and source
plans. It provides no working device feature. The manifest lists the proposed
packages and the checks required before release.

## Package scope

- `uke-camera-support`
- `libcamera-uke`

Nabu reference families: `nabu-camera`, `libcamera`, `nabu-camera-support`.
These are process references; Uke wiring, firmware and runtime behavior require
independent implementation and validation.

## Required validation

- Identify Uke sensors and graph from exact-profile source.
- Keep firmware/calibration private and review licenses.
- Require frame capture evidence before advertising support.

The first packaging target is Fedora Rawhide AArch64 in
[uke-linux-test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/).
DEB and Alpine APK targets require their own native rules later.
Unimplemented targets fail explicitly rather than producing placeholder RPMs.

Use `make validate` to check this repository's manifest contract. Native device
applications use C++; upstream C/assembly interfaces retain their original
languages. Host automation uses Bash or Make. Python does not enter tablet
payloads. Source archives belong in `referances/`, development in `src/`.
Public records exclude personal paths, unit identifiers and calibration data.

See [the roadmap](docs/ROADMAP.md), [source policy](docs/SOURCE-POLICY.md) and
[manifest](manifests/component.json). New material findings are recorded in
`docs/lessons/`; source, package, emulation and physical evidence stay distinct.
