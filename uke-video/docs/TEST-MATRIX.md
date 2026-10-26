# Uke video acceptance matrix

Updated: 26 October 2026. Physical environment: POCO Pad X1, Fedora 46 Rawhide,
Linux 7.3.0-10 for VA queries and 7.3.0-12 for package-only delivery. Mesa
26.2.5-11 and libva 2.24.1 were captured in the query environment. Package and
source checks are separate from codec workload acceptance.

| Check | Observed result | Remaining gate |
|---|---|---|
| Host backend/RPM and capability-query faults | Passed | Native codec workloads |
| Native AArch64 COPR 11204584 | Passed, signed | Codec correctness and application use |
| Native AArch64 revision 0.2 / COPR 11205659 | Signature, payload, one-package DNF upgrade, installed RPM/ELF and dependencies passed | Native VPU and Main10 workloads |
| DNF installation, RPM verification, ELF/hash | Passed on tablet | Native Iris platform |
| Native kernel 7.3.0-6 / COPR 11205247 | Build/signatures, DNF, RPM verification, boot-B readback and recoverable boot passed | Native codec sessions |
| Prior 7.3.0-6 native first open | Drivers bind; decoder/encoder register; MVS0C power-on times out | Configuration clocks, controller power and PAS/HFI |
| Shared kernel 7.3.0-9 / VIDEOCC-only activation | Gate readback passed; display SID 0x1c00 / CB10 fault starts before Iris load; 60 Hz recovery reported by the coordinator | Approved provider-only trace and shared-resource correction before codec trials |
| Coordinated 7.3.0-10 / VIDEOCC-only trace | Idle inhibited; no new display fault in the 16:51:43-16:52:58 UTC capture; provider loaded at 16:52:19, Iris absent | Separate power-transition isolation; no Iris session established |
| Coordinated 7.3.0-10 / Iris load | Nodes register and background clients access the device; USB/DP disconnected four seconds before loading | Stable USB/DP topology, explicit firmware/API proof and approved codec workload |
| VA revision 0.3 / COPR 11205969 | Host/native build fixtures, signature and payload checks passed; not installed separately; its code is included in installed 0.4 | Physical rate/output correctness |
| Private runtime firmware | Local DNF package installed, hash/RPM verification clean | PAS/HFI initialization; no public redistribution |
| Explicit VA initialization on 7.3.0-10 | Revision 0.2 initializes successfully as the desktop user; 7 decode and 4 encode profile/entrypoint pairs; no new non-audit kernel messages | Actual native/VA codec output and browser integration |
| Default VA discovery on 7.3.0-10 | Historical `vaInitialize -1`: default libva searches for absent MSM alias | Preserve before receipt; superseded by the revision-0.4/kernel-14 query below |
| VA revision 0.4 / COPR 11206106 on 7.3.0-12 | Historical package-only DNF upgrade, RPM/ELF and relative MSM alias verified; modules unloaded and no VA query in that window | Later activation/query is recorded below; Chrome and rendering regression remain due |
| Default VA revision 0.4 on 7.3.0-14 | One UID1000 query without overrides opens MSM alias, exits 0 and lists 7 decode/4 encode pairs; DP10/30 remains active; saved kernel excerpt is audit-only | Real native/VA frames and GPU sandbox; separate normal Chrome GL is recorded below |
| Chrome156 normal GL on kernel14 | Canvas/WebGL shader pixels and context restoration pass on FD732; initialized Chrome decode list has eight mapped rows; sandbox false, WebGPU no adapter | Early/fatal sandbox comparison, real codec frames, native buffers and physical presentation |
| VA revision 0.5 / COPR 11206385 | Fresh GPU-enabled host build, all native AArch64 fixtures and signed payload checks pass; not installed | Controlled delivery, VA regression and real boundary-mode output |
| Native kernel15 H264 constrained-baseline320x240 |30/30 NV12 frames match independent software hashes; no new non-audit kernel message | VA/browser, broader modes, lifecycle and sustained acceptance |
| Native kernel15 H264 High/B2 at320x240 | Failed: one corrupt frame and five HFI fatal errors despite FFmpeg exit0; further codecs stopped | Diagnose queues/HFI/stream before retry; Main untested |
| HEVC Main decode | Not run | NV12 reference frames; tiles/WPP/RPS/long-term references |
| HEVC Main10 decode | Not run | P010 precision and independent SDR/HDR metadata |
| VP9 P0/P2 decode | Not run | 8/10-bit references, superframes, alt-ref/show-existing |
| H264 / HEVC Main encode | Not run | Independent decode, RC/GOP/IDR, quality, color |
| HEVC Main10 encode | Native P010 and VA source candidates pass host checks; no native bitstream | Matched delivery, P010 MMAP/GPU/import, independent precision and metadata |
| Main10 patch on kernel-13 source | Patch applies; Main10/caps contracts and Clang-23 AArch64 driver cross-build pass | Whole matched kernel build/delivery and actual encode; no object is installed |
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
| Google Chrome / Firefox / Chromium | Chrome stable 156.0.8078.47 installed; hardware decode not qualified | Exact backend and decoder, sandboxed device access and decode logs |
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
SDR metadata. The native constrained-baseline and High trials above consume their respective inputs; other corpus profiles remain untested on Iris. Broader
references, resolution-change and stress vectors remain due.
