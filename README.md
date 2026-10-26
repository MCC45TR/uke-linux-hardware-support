# Uke graphics and native diagnostics checkpoint

**26 October 2026 — source publication.**

This development branch preserves the graphics/video backend and browser validation work, native identity helper, standalone Vulkan/GLES/OpenCL probes and exact research manifests. These sources remain separate from the default package families.

Publication checks pass for the native identity fixture, research manifests and host compilation of Vulkan and GLES probes. The publication host and checked x86_64 SDK lack OpenCL development headers, so the OpenCL probe was not recompiled here. Its opt-in target requires those headers and the OpenCL loader.

The identity helper remains inactive; serial output is opt-in and its public machine-info path excludes serials. The `Rev:1.1` presentation label is distinct from an observed OEM board revision.

Recorded device observations include GL pixels and default VA initialization. On 26 October, Linux 7.3.0-15 produces 30 exact-reference native H.264 constrained-baseline frames at 320x240 with locally provisioned VPU firmware; the High trial fails with a corrupt frame and firmware errors. Other codecs, encoding, the corrected Vulkan artifact and sandbox/GL coexistence remain unqualified. Earlier Chromium 154 X11/Vulkan WebGPU tests pass, while current normal Chrome 156 returns no adapter. The installed VA 0.4 baseline and uninstalled 0.5 candidate retain separate delivery records.

No COPR request, package installation, browser/device workload or firmware operation was performed by this publication.

[Current platform status](https://github.com/MCC45TR/uke-linux#hardware-support) · [Default package source](https://github.com/MCC45TR/uke-linux-hardware-support/tree/main) · [Graphics/video source notes](uke-desktop-integration/README.md)

Original distribution/upstream licenses and human contributor attribution are retained.
