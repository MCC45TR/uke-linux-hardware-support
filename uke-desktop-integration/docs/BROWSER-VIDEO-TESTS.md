# Browser graphics and video validation

Date: 26 October 2026. Target: POCO Pad X1, original Google Chrome
156.0.8078.47, Chromium source 1d17a2e2ee147bb7d6ae2ab6abdb3a2510c17775.
The supplied GPU report has SHA-256
539485ce3097f487d8226ebf1066774b0bdd7fe889c579f5954e68f29e369869.
Source, host fixtures, packages and tablet workloads are separate results.

On26October, kernel15 normal Chrome passes FD732 Canvas/WebGL pixels and
context restoration with GPU Seccomp0. The early/fatal comparison enables
Seccomp2/one filter but loses hardware GL. A native H264 constrained-baseline
320x240 session produces30/30 exact NV12 reference frames; a separate High/B2
session fails with firmware fatal errors and one all-zero error buffer.
Later untraced VA0.4 sessions on the same kernel pass all seven320x240 decode
profiles with30/30 exact NV12/P010 reference frames. Native High/B2 remains
tracing-sensitive and fails without strace. Chrome submits no accepted video
frame; encode, WebGPU and physical display acceptance remain open.

The [Linux ARM sandbox candidate](CHROMIUM-ARM-SANDBOX-2026-10-26.md)
contains DRM/EGL and direction-gated Iris node policies with two passing host
function fixtures. Exact156 source/SRPM checks, full native COPR11208913 and
both RPM signature/payload checks pass. No candidate binary is installed or
qualified on the tablet. Preserve the original
Google Chrome and invoke the candidate binary directly for the later controlled
trial; Fedora's wrapper configuration contains unqualified feature forces.

The later kernel16 uke2 early/fatal trial fails: its first hardware GPU crashes
on an optional Mesa sibling-worker scheduling request, and the replacement
has Seccomp2 but disabled GL. A separate GBM module read is denied. The uke3
follow-up admits the exact module read, refuses the optional batch hint with
EPERM and keeps stock graphics features. Host source/config checks pass;
native COPR11210239 and subsequent device acceptance remain pending.

## Coverage of the supplied feature table

| Feature | Reported state | Action and acceptance evidence |
|---|---|---|
| Canvas | Hardware accelerated | Preserve baseline; existing GPU page checks pixels. Compare before/after each change. |
| Direct Rendering Display Compositor | Disabled | Keep disabled. Exact source explicitly excludes Linux support; do not force DrDC. |
| Compositing | Hardware accelerated | Preserve selected hardware renderer; collect GPU process, report and visible presentation evidence. |
| Multiple Raster Threads | Enabled | Retain. This label alone does not identify the threads present when GPU sandbox startup fails. |
| OpenGL | Enabled | Retain ANGLE/OpenGL baseline and Mesa's angleisbroken workaround. Test shader pixels and context restoration. |
| Rasterization | Hardware accelerated | Preserve baseline; compare the same visible workload, resource use and frame timing. |
| Raw Draw | Disabled | Optional rendering path; no video requirement justifies forcing it. |
| Skia Graphite | Disabled | Source-supported experimental comparison after sandbox/GL acceptance; separate isolated profile and measured regression. |
| TreesInViz | Disabled | Optional compositor architecture; not a missing codec implementation. |
| Video Decode | Hardware accelerated label | Empty profile list and vaInitialize failure take precedence. Require real native/VA/Chrome frames per codec/profile. |
| Video Encode | Software only | Qualify backend output first, then the exact AcceleratedVideoEncoder feature and real WebCodecs/WebRTC output. |
| Vulkan | Disabled | Compare a scoped Vulkan profile with repaired GL; record the path actually selected. Disabled compositing does not mean Turnip is absent. |
| WebGL | Hardware accelerated | Existing GPU page checks clear/shader pixels and context restoration; keep complete failure results. |
| WebGPU | Hardware accelerated label | Existing page checks selected adapter, compute and render. New video page checks decoded-frame external textures separately. |
| WebGPU interop | Disabled | Preserve blocklist entry 186. An API import or a CPU copy does not qualify native Vulkan/GL sharing. |
| WebNN | Disabled | Separate inference subsystem; no admitted native NPU/WebNN requirement for this video work. |

The many ANGLE/Dawn toggles and disabled extensions are compatibility settings,
not a checklist to enable. CPU adapters listed by Dawn are not evidence that
the active renderer uses them.

## Remaining diagnostics and implementation

| Evidence | Completed preparation | Remaining tablet gate |
|---|---|---|
| vaInitialize failure | Default revision-0.4 query passes on kernel14 as UID1000 without overrides; real MSM alias/ABI/backend loaded, 7 decode/4 encode pairs | Preserve this standalone result; Chrome sandbox, VA output and profile/lifecycle qualification remain due |
| Sandboxed:false / multiple threads | Kernel15 normal GL passes with Seccomp0; early Seccomp2 loses GL. Traced DRM/EGL broker denials lead to the source candidate | Complete native build and sandboxed hardware GL regression; no flag-only repair |
| Empty decode/encode lists | Initialized Chrome decode list now has eight rows; encode stays empty; dimension0.5 is built but uninstalled | Actual sessions/frames and native encoder qualification; do not use an initial asynchronous empty sample as final |
| Encoder policy | Exact Linux AcceleratedVideoEncoder name is pinned; WebCodecs and WebRTC probes prepared | Backend output first, then scoped feature trial with actual implementation/Iris trace |
| 10 bpc/PQ versus 8 bpc/gamma | Saved kernel-14 output-bits and KScreen states correlated below | Complete sink/format/color validation and window/video moves between qualified HDR/SDR outputs |
| DMA-BUF, modifiers and fences | Real decoded-frame WebGPU test; source copy/fence path identified | Native buffer identity/layout, import/export lifetime and synchronization trace under actual workload |
| One-copy | Kept as compositor tile metadata | Measure video copies separately; backend persistent-surface copy is explicit in source |
| renderD128 override | Kernel14/15 boot repeats Adreno by-path identity; Chrome adds override itself and FD732 GL pixels pass | Actual video buffer regression and cold-boot video activation remain due |

For surfaces exported before decode, the backend's persistent_export branch
copies Iris CAPTURE storage into persistent exported storage. Its GPU path
waits for source/destination DMA-BUF access, performs GL copies and finishes
before publishing. Its explicit CPU alternative is a different path. The
other decode branch shares CAPTURE storage directly. Trace which branch the
actual client uses; source alone does not count its runtime copies. Removing
the persistent-export copy requires validated ownership/lifetime changes,
which a browser zero-copy switch cannot establish.

Saved coordinator receipts from kernel 7.3.0-14 on 26 October report both
DP-1 and DSI-1 active at 10 bits/component and 30 bits/pixel. DP DSC is off;
DSI DSC uses 128/16 compressed bits/pixel, which does not turn its component
depth into 8 bits. DP runs two HBR2 lanes and reports hdr_requested=0.
KScreen reports DP enabled, maxBpc=10 and hdr=false. DSI is enabled but omits
HDR state and reports maxBpc=0; those fields must not replace its kernel-active
10-bpc state. This correlates the saved kernel and compositor output state.
It does not establish HDMI-sink precision, active HDR, per-surface tone mapping
or window/video moves. No new Chrome report or target access is claimed here.

A later coordinator-owned default VA query at 04:23:02 UTC, after a kernel-14
reboot, records DP active at 10 bpc / 30 bpp and DSI inactive. These are different
boot epochs; do not turn the earlier two-output snapshot into the current
state or attribute the DSI difference to the VA query without a matching
before/after display transition trace.

## Prepared workloads

Only copy browser-video.html and browser-video.js to the reviewed temporary
test directory on the tablet. The .mjs tools below run on the host. No Node
or Python runtime is a tablet test dependency. The page starts no workload
until a button or scoped CDP call invokes it.

WebCodecs encode generates thirty 640x360 frames. H264 constrained baseline,
Main/High and HEVC Main use CPU NV12. The separately gated HEVC Main10 case
uses little-endian I420P10 with nonzero low two bits; it does not relabel an
8-bit source. Independent decoded precision remains required. The size exceeds
Chrome 156's default 321x241 VA encode performance floor. Keep that policy
enabled; a 320x240 encode test can legitimately select software.

Decode accepts the reviewed thirty-frame 320x240 corpus: H264, HEVC Main/Main10
and VP9 Profile 0/2. It retains native frame format, layout, crop, color, PTS
and SHA-256 rather than converting everything to 8-bit RGBA. Its hashes pack
visible planes without stride padding. Imported-frame
comparison is opt-in with input.interop=true. It compares a decoded frame's
WebGPU external-texture RGB output with CPU RGBA conversion at tolerance two.
This is a functional conversion check, not 10-bit/HDR precision or zero-copy
acceptance. WebGPU failure remains a separate failed result.

WebRTC uses local peer connections without camera, microphone, STUN servers
or external signaling. H264/H265 must appear in actual browser capabilities;
codec preferences prohibit a VP8/VP9 fallback. Retain selected fmtp,
frames encoded/decoded, implementation names and available efficiency/timing
statistics. Those fields and prefer-hardware remain insufficient VPU proof.

The recorder stores elapsed and callback latency, including CPU upload/copy
overheads. Thirty frames and synthetic PTS do not establish sustained FPS.
Keep kernel, CPU/GPU/VPU, thermal and presentation measurements alongside it.

## Host preparation and checks

Run from this component:

    node tests/browser-video-host.mjs
    node tests/pack-webcodecs-host.mjs
    node tests/validate-browser-decode-host.mjs
    node tests/unpack-webcodecs-host.mjs

These are API/packet fixtures; they do not run a real codec. To prepare actual
packet jobs, use the existing software-generated corpus and FFprobe on the host:

    ffprobe -v error -select_streams v:0 -count_frames -show_streams \
      -show_packets -show_data -of json input.mp4 > packets.json
    sha256sum input.mp4
    node tests/pack-webcodecs-corpus.mjs packets.json STREAM_SHA256 > decode-job.json

The packer validates packet lengths, timestamps, contiguous hex data and
AVC/HEVC configuration records. It preserves B-frame PTS and derives codec
strings rather than labelling all content H264 baseline. Decode job files
contain coded stream bytes and must remain private test artifacts.

For pixel acceptance, create a same-input software reference in the browser's
declared native format. Use FFmpeg framehash with SHA-256 and retain source
SHA-256, visible dimensions, pixel format, file name and the reference file's
own SHA-256 in its reference JSON. Then run:

    node tests/validate-browser-decode.mjs capture.json reference.json

The validator checks all thirty visible-plane hashes, PTS, format, dimensions
and byte counts. Changed input, repeated PTS and mismatched pixels fail.
Successful reference comparison still reports hardwareAccepted=false until
actual Iris/Chrome evidence qualifies that path. Seven real software corpora
and fourteen native-format reference sets are prepared privately; these are
host software results, while browser/WebGPU/WebRTC device results remain open.

Encode job: {"operation":"encode","codec":"avc1.42E01E"}.
WebRTC job: {"operation":"webRtc","codec":"video/H264"}.
Extract a completed WebCodecs encode capture on the host:

    node tests/unpack-webcodecs-encode.mjs capture.json NEW_OUTPUT > encoded-receipt.json

The extractor refuses existing output files, malformed or length-prefixed data,
missing outputs and repeated PTS. It retains coded order when PTS reorder, and
binds the result to its SHA-256. Decode that exact file with the independent
software decoder and inspect all thirty frames, visible dimensions, actual
codec/profile/bit depth and color. For example, for an H264 result:

    ffprobe -v error -f h264 -count_frames -show_streams -of json NEW_OUTPUT
    ffmpeg -v error -nostdin -hwaccel none -f h264 -i NEW_OUTPUT \
      -an -f framehash -hash sha256 -n encoded-output.framehash

Use hevc as the demuxer for HEVC. Neither extracting valid-looking NAL units
nor an independent decode proves native hardware use, fidelity or genuine
10-bit precision. Retain the original capture and measured encoder evidence.

The host CDP observer captures the diagnostic page and SystemInfo before/after,
only enables Media observation for its own test target and closes only targets
it created. Exact browser/revision guards remain mandatory.
Each report is read from a fresh diagnostic target. Missing report sections
or failed GL pixels/context restoration return a failed process status;
WebGPU remains an independent result.

Run the controller on the host through the trial's loopback SSH forward:

    node tests/capture-browser.mjs FORWARDED_PORT \
      file:///var/tmp/uke-browser-video-20261025/browser-video.html JOB.json > capture.json

For the unchanged GPU baseline page, use browser-gpu.html without a job file.
The controller never starts or shuts down the browser; the scoped trial
supervisor controls only its isolated profile and process.

## Ordered comparison

1. Default VA and real native/VA output.
2. Normal GL report versus early/fatal-sandbox GL; no codec stream in this startup comparison.
3. Codec/profile decode and backend encode correctness, then browser encode.
4. Same repaired GL configuration versus a Vulkan feature profile.
5. Same configuration versus SkiaGraphite with its Vulkan Dawn backend.
6. Decoded-frame imports and physical HDR/SDR moves in their own windows.

Vulkan and SkiaGraphite names/options are present in the pinned source. Never
combine them with global blocklist bypass or force WebGPU interop. Retain
unavailable/rejected paths and regressions. Each trial needs its own report,
actual backend logs and the same input/reference measurements. A host fixture,
successful package build or completed JavaScript promise cannot pass a tablet
acceptance gate.
