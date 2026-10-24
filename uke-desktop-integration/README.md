# uke-desktop-integration

Uke rotation, cover, brightness, power and audio desktop policies for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

This repository contains platform policies and their packaging. The manifest
describes the wider integration scope; individual hardware results are recorded
separately.

## Package scope

- `uke-desktop-integration`
- `uke-displayport-audio`: digital-only ALSA UCM and protected THP discovery policy.

On 24 October 2026, POCO Pad X1 with kernel 7.2.12-31 and signed policy 0.1.0-3
plays native stereo PCM and Chromium video sound through Juo JH925 to a
ViewSonic VA3209-QHD. Internal speakers, capture, reconnect endurance and the
other integration policies are not established by that result.

## DisplayPort monitor labels

`make pipewire-monitor-srpm FEDORA_PIPEWIRE_SRPM=<original-source-rpm>` applies
the bounded monitor-label patch to the exact Fedora PipeWire 1.6.10-1.fc46
source RPM. The host helper checks its SHA-256 before extraction and retains
Fedora's build flags, dependencies, tests and package family. The added check
exercises the actual route collector with two monitor models, absent/empty
ELD, an unrelated card and separate active device/profile selections.

The patch uses actual ELD product names only for `POCO-Pad-X1-HDMI`. It changes
Pulse sink/port labels without changing audio formats, routing or EDID. The
matching UCM ELD mapping and a separate tablet label test are required. Rebase
and validate this patch when Fedora changes the PipeWire source version.
No KDE application is copied or rebuilt. Fedora's existing Meson and docutils
build tools are host-only upstream dependencies; their implementation is
retained rather than replaced. No project Python code or Python target payload
is introduced.

Nabu reference families: `nabu-desktop-integration`.
These are process references; Uke wiring, firmware and runtime behavior require
independent implementation and validation.

## Required validation

- Use measured Uke input and sensor paths.
- Require physical suspend, charger and routing evidence.
- Keep generic distribution defaults until behavior is measured.

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
