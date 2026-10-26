# Uke stateful video integration

Date: 26 October 2026. Target: POCO Pad X1 / Xiaomi Pad 7, SM7675.

COPR publication now belongs to the existing `uke-additional` source family.
The child RPM keeps the name `uke-vaapi-v4l2`; the standalone source-package
record and its six historical builds are retired after source/rollback
preservation. Family source3e83bf0 produces additional1.0.0-10 and backend
1.0.0-1 from the reviewed revision0.6 code. All36 planned RPM names start with
`uke-`. Native AArch64 build11210929 succeeds in the existing test directory
and produces34 RPMs; the two empty parent declarations produce no binary.
All34 signatures, header/payload digests, source-family identities and payload
audits pass. No Python payload, direct Python requirement or Python scriptlet
is found. The five backend fixtures pass. The libaccounts-glib suite reports
a D-Bus session failure that the inherited recipe tolerates; this is not an
all-tests-passed result. No new source family or Chromium rebuild is introduced.
Tablet delivery and its resolved dependency/render/backend regressions remain
open. Future FFmpeg integration also belongs to
the existing family; the source-only client candidate below is not published
as an independent COPR package.

The Mesa self-target scheduling correction advances the same family to
1.0.0-11, Mesa to 26.2.5-14 and the unchanged backend code to 1.0.0-2.
COPR 11211213 succeeds on 26 October. All 34 binary signatures, source-family
headers and payload audits pass; the backend's relative alias, lack of
scriptlets and working0.4 dependency set remain intact. The real Mesa queue
fixture passes all four cases in both native build/check phases, retaining
512 jobs per run and the modeled self-only scheduling restrictions. The
inherited libaccounts D-Bus failure remains. No candidate is installed on Uke;
this does not establish original Chrome sandboxed GL or codec acceptance.

Default VA initialization passes on Fedora and Linux 7.3.0-14/15 with signed
revision 0.4. One coordinator-owned query as UID1000, without driver/path/device
overrides, loads the MSM alias and returns seven decode/four encode profile
entrypoint pairs at 04:23:02 UTC. The earlier default query failed because that
alias was absent. Profile-wide acceptance remains open.

On26October, kernel7.3.0-15 and installed VA0.4 produce30/30 exact independent
reference frames for all seven decode profiles at320x240. HEVC Main10 and
VP9 Profile2 preserve full P010 precision. These are bounded, untraced Iris
hardware sessions with explicit CPU download. Browser playback, direct display,
mode limits, lifecycle and sustained acceptance remain open.

The native FFmpeg H264 constrained-baseline path also passes. Native High/B2
fails without tracing but produces correct frames with strace, so its startup/
source-change sequencing remains under investigation. Native H264 Main encode
produces30 independently decodable640x360 frames with correct30fps SPS timing,
but lacks requested color fields. A raw ffprobe rate field initially misled
the cadence interpretation; direct SPS arithmetic corrects it. Encoding is not
qualified by output alone; no browser encode feature is enabled from this result.

Iris and VIDEOCC are loaded in this controlled window. Iris uses video1/video2;
video0 belongs to NT36532 touch. DP remains active at 10 bpc / 30 bpp after the
query, and the saved kernel interval contains only audit-related messages.
A separate normal Chrome156 trial on kernel14 passes FD732 Canvas/WebGL pixels
and context restoration; its initialized decode list has eight mapped rows.
GPU sandbox remains false and WebGPU obtains no adapter. The physical monitor
was already black; pixel readback does not qualify visible presentation. A prior release-9
display fault remains unresolved; shared-display codec workloads and lifecycle tests
are separate gates. Cold-boot module activation is not qualified by this
explicit-load trial. Further experiments require a controlled hardware window.

The experimental package pins Radxa libva-v4l2 at
`6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41`. It intersects the implemented
profiles with V4L2 coded formats, raw formats and profile menus. This is API
availability, not decoded-frame or encoded-bitstream acceptance.

The tested default-discovery command is:

```sh
env -u LIBVA_DRIVER_NAME -u LIBVA_DRIVERS_PATH \
  -u IRIS_VAAPI_DEVICE -u IRIS_VAAPI_ENCODER_DEVICE \
  vainfo --display drm --device /dev/dri/renderD128
```

Revision 0.4, COPR 11206106, replaces installed revision 0.2 in a one-package
signed DNF transaction. It retains the upstream MSM alias so default discovery
works without an environment override in the controlled kernel-14 query.
It uses a separate test repository. Codec, sandbox, rendering and application
acceptance remain required before normal-channel promotion.
FastCV/CDSP VPP is disabled. Revision 0.1 does not advertise Main10 encode.
The installed revision retains revision 0.2's P010 input and matching
HEVC Main10 parameters;
it advertises that profile only when the kernel exposes both P010 and the
Main10 profile menu. Native bitstream, precision and GPU/import validation
remain open. AV1 has no admitted Uke source/runtime evidence.

Source revision 0.3 replaces the fixed 240-fps and 245-Mbit/s encode checks
with runtime bitrate-control bounds and returned Q16 frame-rate validation.
Host checks cover 480-fps requests, fractional rates and driver refusal or
substitution. Native AArch64 COPR 11205969 and its build fixtures pass; the
signed 0.3 package is not installed separately, while its code is included in
installed 0.4. These checks do not establish
tablet throughput. The fixed VA dimension
bounds and the full native/application test matrix remain open.

Revision 0.5 is a separate source candidate. It queries V4L2 frame-size bounds
per codec/direction and uses them in VA attributes, surface queries and encode
validation. A fresh GPU-enabled host build and profile/dimension, VA-query,
Main10/P010 and rate fixtures pass. Native AArch64 COPR 11206385, signature,
payload and unchanged runtime dependencies also pass. It is not installed. These axis bounds do
not promise every width/height pair or sustained rate; kernel macroblock
admission and measured output remain required. Discrete-only enumeration is
not advertised. Installed0.4 remains the default-query and bounded VA decode baseline.

Revision0.6 retains the runtime dimension changes and preserves decode-error
status after damaged-buffer sanitization. The old-source regression fails and
the actual-function correction passes. Native AArch64 COPR11209327, all five
fixtures, trusted RPM signatures and payload audits pass. No Python or scriptlet
is included. Revision0.6 is not installed; healthy/damaged device sessions and
render/browser regressions remain required.

A separate FFmpeg9.0.2 source candidate preserves startup errors instead of
reporting success without a frame. It retains an unqueued packet on EAGAIN.
The signed RPM Fusion source package and upstream archive verify; all six
V4L2 files match the official tag after the distribution's three patches.
Actual-function tests fail on the original source and pass after the change;
the complete modified C translation unit compiles on x86_64. This patch is
not part of the libva package. No replacement FFmpeg package is built or
installed, and initial source-change/DRC ordering is still unresolved.
Source identities and validation limits are in
[the candidate manifest](manifests/ffmpeg-start-error-candidate.json).
Run its host fixture with
`bash tests/ffmpeg-start-error-contract.sh <prepared-FFmpeg-source>`.

A separate streaming candidate combines the open upstream DRC/buffer
series with initial-metadata sequencing and error handling. Minimal x86_64
libraries compile with nine V4L2 decoder and five encoder wrappers; these
wrapper counts describe build coverage, not Uke codec support. Actual-C host
fixtures cover full output queues, partial input, LAST/EPIPE transitions,
cloned plane references, concurrent final releases and replacement pools.
They use modeled V4L2 ioctls and real host anonymous mappings/refcounts.
Fixture assertions are explicitly re-enabled after upstream headers, which
otherwise disable libc assertions. No replacement FFmpeg RPM is built or
installed. Native untraced frames, driver orphan-buffer support, full lifecycle
and sustained acceptance remain open. The prototype's separate scope and
input hashes are recorded in the same candidate manifest.

The streaming patches are retained for review under `patches/`. Start with
the exact distribution-prepared 9.0.2 source, before the startup candidate:

```sh
patch --batch --forward --fuzz=0 -d "$ffmpeg_source" -p1 < patches/ffmpeg-9.0.2-preserve-start-errors.patch
patch --batch --forward --fuzz=0 -d "$ffmpeg_source" -p1 < patches/ffmpeg-9.0.2-upstream-buffer-drc.patch
patch --batch --forward --fuzz=0 -d "$ffmpeg_source" -p1 < patches/ffmpeg-9.0.2-initial-source-sequence.patch
```

The middle patch ports Nick Rogers' retained PR25012 series. The final patch
adds initial-metadata sequencing and error handling; neither is device-qualified.
A fresh archive replay reproduces every V4L2 source file of the tested host
prototype. Forward-only application rejects an already-patched input instead
of silently reversing it. The existing working control tree already includes
the startup-error fix, so it must not be labelled stock FFmpeg. None of these
FFmpeg patches is applied to the VA backend or published as a standalone RPM.

Same-context seek is still a separate implementation gate: the retained
V4L2 decoder descriptor has no flush callback, and the streaming patches do
not reset its driver/drain state through `avcodec_flush_buffers()`. A new
process that decodes successfully does not qualify seek within an existing
decoder context. Keep this requirement open when testing the candidate.

On kernel 7.3.0-10, explicit `vainfo` exposes H264 Constrained Baseline/Main/High and HEVC
Main decode/encode, plus HEVC Main10 and VP9 Profile 0/2 decode. Native Main10
encode remains gated out. No new non-audit kernel message appears after either
query. This proves backend initialization and queried API availability, not
frame production or Chrome acceleration.

Build requires the original distribution Meson 1.12.1-2.fc46, which uses
Python on the build host. Meson is the upstream build system; replacing it
would create an unnecessary parallel build recipe. No Python file, build
tool or Python runtime is included in the video RPM.

The native Linux 7.3 platform candidate is integrated into the normal source
preparation and built as kernel 7.3.0-6 in COPR 11205247. The signed matched
RPMs are installed with DNF; RPM verification and the native current-slot
preflight and recoverable boot pass. Shared release 9 retains that source and
the gate correction, but the provider-only display failure remains open.
The exact private runtime
firmware is installed through a local DNF package and stays outside the COPR
source RPM.

Remaining work: qualify native firmware/HFI under workload, validate
H264/HEVC/VP9 sessions and end-to-end Main10 encode, and execute the
codec, lifecycle, performance and application matrix from the research report.

On 26 October, the unchanged native Main10 patch also applies to saved
kernel-13 Iris source. Main10/capability contracts and a Clang-23 AArch64
driver cross-build pass. This does not build or install a complete kernel;
Main10 encode remains unavailable until a matched kernel delivery and real
P010/bitstream validation pass.
