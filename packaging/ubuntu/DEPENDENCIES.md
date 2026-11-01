# Tenacious ARM64 dependency evidence

Observed 1 November 2026 from the official Ubuntu ports indexes:

- https://ports.ubuntu.com/ubuntu-ports/dists/tenacious/main/binary-arm64/Packages.xz
- https://ports.ubuntu.com/ubuntu-ports/dists/tenacious/universe/binary-arm64/Packages.xz

`tenacious-dependencies.tsv` records selected package versions/architectures.
Development-series versions can change; recheck before building or upload.

Fedora `alsa-lib-devel`, `wayland-devel`, `bluez-libs-devel`, `systemd-devel`,
`systemd-udev`, `alsa-ucm` and `pipewire-pulseaudio` map to Ubuntu
`libasound2-dev`, `libwayland-dev`, `libbluetooth-dev`, `libsystemd-dev`, `udev`,
`alsa-ucm-conf` and `pipewire-pulse`. Runtime ELF dependencies use Debian
`dpkg-shlibdeps`, not manually copied RPM dependency strings.

Ubuntu includes libssc 0.4.4-2 and SensorProxy 3.9-1ubuntu1, matching upstream
versions. The shared Uke patches still add required behavior. Debian recipes
retain libssc's original libssc2/libssc-bin/libssc-dev/gir1.2-ssc-2 binary names,
and SensorProxy's original name. Versions sort above the observed archive
versions. Semantic Uke ABI virtual dependencies prevent unpatched Ubuntu
libraries or SensorProxy from satisfying the SSC stack accidentally.

Meson 1.10.1-1ubuntu2 exceeds libssc's declared minimum 1.4.0. Its host-only
Python implementation, protobuf generators and GObject introspection are
unmodified upstream build tools, not tablet payloads. Replacing these build
systems would introduce an unrelated source fork. Build execution is not yet
authorized here. Python mock servers remain disabled; recipes reject Python
files from installed runtime staging. No claim about the full runtime
transitive dependency closure is made before target package resolution.

PipeWire 1.6.8-1ubuntu4 is below the shared audio recipe's patched 1.6.10
requirement. `uke-displayport-audio` therefore requires both >=1.6.10 and the
future `uke-pipewire-audio-abi-1` provider; it cannot silently install against
an incompatible stock version. Speaker controls similarly require the future
`uke-kernel-speaker-controls-abi-1` provider. These capabilities must be emitted
only by recipes carrying the matching reviewed changes.

PowerDevil/libksysguard 6.7.5 and ddcutil 2.2.7 are below adaptive-display's
6.8.0 and 3.0.2 requirements. Do not lower the gates or fork KDE. That component
remains deferred pending a suitable original Ubuntu dependency set.

SSC firmware/configuration must supply `uke-ssc-firmware-inputs-abi-1` only
after its exact firmware/config payload is admitted. Neither calibration nor
private touch data may enter a public source upload. The public source
preparation allowlist rejects `uke-local-touch-data` explicitly.

UPower is already 1.91.5-1 in Tenacious. The overlay retains its five shared
battery/history patches and original `upower`, `libupower-glib3`,
`libupower-glib-dev`, and `gir1.2-upowerglib-1.0` binary names. Standard Polkit
and systemd behavior is retained. The charge-control and offline-charge
binaries remain separately gated by the corresponding kernel and boot ABI
providers; defining their packages does not qualify a charging writer.

A name-level check of every declared Build-Depends against these ARM64 indexes
found all concrete package names. `debhelper-compat (= 13)` is provided by
Tenacious debhelper 14.5ubuntu1. This check does not replace an APT solver or a
clean native build: the patched libssc-dev minimum intentionally exceeds stock
libssc-dev, and ABI providers must be built in dependency order.
