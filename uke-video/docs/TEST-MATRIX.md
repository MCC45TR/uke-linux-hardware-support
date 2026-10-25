# Uke video acceptance matrix

Date: 25 October 2026. Physical environment: POCO Pad X1, Fedora 46 Rawhide,
Linux 7.3.0-5, Mesa 26.2.5-11, libva 2.24.1. Package and source checks are
separate from codec workload acceptance.

| Check | Observed result | Remaining gate |
|---|---|---|
| Host backend/RPM and capability-query faults | Passed | Native codec workloads |
| Native AArch64 COPR 11204584 | Passed, signed | Codec correctness and application use |
| DNF installation, RPM verification, ELF/hash | Passed on tablet | Native Iris platform |
| Native kernel 7.3.0-6 / COPR 11205247 | Build/signatures, DNF installation, RPM verification and current-slot preflight passed | Recoverable boot and native codec sessions |
| Private runtime firmware | Local DNF package installed, hash/RPM verification clean | PAS/HFI initialization; no public redistribution |
| Explicit VA initialization | Failed: no Iris stateful H264 decoder | Platform, firmware, HFI and codec nodes |
| H264 constrained baseline, Main/High decode | Not run | NV12 reference frames; CAVLC/CABAC, B/reference/slices/crop |
| HEVC Main decode | Not run | NV12 reference frames; tiles/WPP/RPS/long-term references |
| HEVC Main10 decode | Not run | P010 precision and independent SDR/HDR metadata |
| VP9 P0/P2 decode | Not run | 8/10-bit references, superframes, alt-ref/show-existing |
| H264 / HEVC Main encode | Not run | Independent decode, RC/GOP/IDR, quality, color |
| HEVC Main10 encode | Not implemented end to end | Kernel P010/HFI/allocation and backend precision/metadata |
| Advanced encode controls and mode limits | Not run | B/ROI/LTR/layers/intra-refresh, source/runtime intersection |
| 4K60, 4096, minimum-size and high-FPS modes | Not run | Correctness followed by sustained tablet measurements |

Every accepted codec/profile still requires 100 open/stream/drain/close cycles,
30 minutes at each claimed target mode, seek, EOS/drain, resolution and bit-depth
changes, malformed/unsupported input, killed-client recovery, reference/pool
pressure, two-session workloads, runtime PM and an explicit suspend policy.
Keep default display-order behavior separate from opt-in decode-order behavior.

Surface acceptance covers create/import/export/derive/get/sync, fd/fence and
allocation lifetimes, pitch/crop/modifiers and pixel preservation. Record CPU
download, GPU copy, direct DMA-BUF display, CPU upload and transcode separately.
Color/HDR acceptance requires pixel values and bitstream metadata, not only
container labels. Protected content, HEIC still images and camera bring-up are
separate from the clear video codec result.

| Application path | Installed baseline / result | Required evidence |
|---|---|---|
| FFmpeg native V4L2 and VA | Signed RPM Fusion 9.0.2-1 installed; H264/HEVC/VP9 decoder wrappers and VA encoders present | Exact backend/node, hardware surfaces and frame comparison |
| GStreamer native V4L2 and modern VA | Good/Base/Bad 1.28.8; VA plugin installed with zero features | Runtime registration, pipeline, frames and no silent fallback |
| mpv / VLC | Not qualified | Exact distribution package, VA and presentation/copy logs |
| Firefox / Chromium | Not qualified | Exact package/build, sandboxed device access and decode logs |
| OBS / Wayland recording | Not qualified | H264 SDR first, independent output decode, color and A/V sync |

Each workload receipt must retain input SHA-256/corpus identity, command,
driver/package/firmware identity, profile/bit depth, coded and visible size,
crop, frame count/timestamps, bitrate/GOP/references, expected output, thermal
state, power policy, copy path, CPU/RSS, duration and observed errors. Empty
workload results in `manifests/capabilities.json` are intentional pending work.

A private software-generated smoke corpus contains 30-frame 320x240 H264
Constrained Baseline/High, HEVC Main/Main10 and VP9 Profile 0/2 streams.
The Profile 2 stream preserves actual 10-bit sample values through independent
software decoding, with all 30 P010 frame hashes matching its input. Main10
HEVC references retain non-8-bit values; a separate input has explicit BT.709
SDR metadata. These are test inputs; no Iris codec processed them. Broader
references, resolution-change and stress vectors remain due.
