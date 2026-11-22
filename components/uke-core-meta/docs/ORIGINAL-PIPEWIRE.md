# Original Fedora PipeWire

Core21 replaces the eight Uke audio runtime packages with original Fedora
PipeWire packages. It keeps the existing ALSA, PulseAudio, JACK and libcamera
interfaces, WirePlumber, UCM, speaker controls and kernel/firmware dependencies.
Normal DNF upgrades replace the standard eight-package runtime together.

The retired fork's only code delta changes ELD labels when the ALSA long name
is exactly `POCO-Pad-X1-HDMI`. Uke's combined card reports `POCO Pad X1`; this
predicate does not apply. Hardware formats, DSP protection, routing and device
lifecycle remain kernel/ASoC responsibilities. This migration adds no daemon,
desktop fork or alternative sound server. Runtime physical acceptance is separate.

Optional packages are not silently obsoleted. If an optional Uke PipeWire package
is installed, select its original counterpart in the same DNF transaction. The
package names map by removing the `uke-` prefix. For example:

```sh
sudo dnf install --allowerasing uke-core-meta pipewire pipewire-alsa \
  pipewire-libs pipewire-pulseaudio pipewire-utils pipewire-plugin-libcamera \
  pipewire-jack-audio-connection-kit pipewire-jack-audio-connection-kit-libs \
  pipewire-gstreamer
```

Include each installed optional counterpart, such as `pipewire-devel`,
`pipewire-doc`, `pipewire-v4l2`, `pipewire-plugin-jack`, `pipewire-plugin-vulkan`,
`pipewire-config-raop`, `pipewire-config-rates`, `pipewire-config-upmix`,
`pipewire-jack-audio-connection-kit-devel`, `pipewire-module-ffado`,
`pipewire-module-filter-chain-lv2`, `pipewire-module-filter-chain-onnx`,
`pipewire-module-filter-chain-sofa`, `pipewire-module-roc` or
`pipewire-module-x11`. Review the DNF plan first with `--assumeno`; replacing
these names must not erase an unrelated application or hardware provider.

UCM, session policy and user PipeWire/WirePlumber configuration are retained.
Before repository cleanup, retain the exact former runtime/Core RPMs needed
for a tested reverse transaction. Recovery does not require a new disk image
or any change to the kernel, A/B slots, calibration or persistent audio policy.
