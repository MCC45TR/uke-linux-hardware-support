# Uke stateful video integration

30 October 2026. Experimental Iris VA-API integration for POCO Pad X1 on
Fedora Rawhide AArch64. The independent `uke-vaapi-v4l2` source producer builds
one runtime package; no umbrella package or Chromium rebuild is used.

The source pins Radxa libva-v4l2 at
`6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41` and retains runtime capability,
Main10, dimension, error and shared-surface cache fixes. Decoder exports use
the existing synchronous CPU-copy path by default; encoder copying retains
its previous policy. Explicit CPU/GPU comparison overrides remain available.
Decode completion, DMA-BUF fences and cache maintenance are preserved.

Native UHD decoder activity is reported asynchronously to the current user's
D-Bus video policy. Ordinary Chrome sessions can receive temporary CPU and
standard power-profile hints without selecting a website, profile or launcher.
The dependent `uke-video-session` package supplies per-user activation and
recovery. Policy unavailability must not turn a valid decode into an error.

Native pixel controls on kernel 7.3.0-54 with the previous backend match 60
NV12 and 60 P010 references through CPU copying. They do not qualify the new
integration, HDR output or sustained browser presentation. Remote build,
installed ordinary-Chrome playback and lifecycle acceptance remain separate
checks. Historical package and hardware records are in [CHANGELOG.md](CHANGELOG.md).
