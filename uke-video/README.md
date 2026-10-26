# Uke stateful video integration

Date: 26 October 2026. Target: POCO Pad X1 / Xiaomi Pad 7, SM7675.

Default VA initialization now passes on Fedora and Linux 7.3.0-14 with signed
revision 0.4. One coordinator-owned query as UID1000, without driver/path/device
overrides, loads the MSM alias and returns seven decode/four encode profile
entrypoint pairs at 04:23:02 UTC. The earlier default query failed because that
alias was absent. No codec workload is accepted.

Iris and VIDEOCC are loaded in this controlled window. Iris uses video1/video2;
video0 belongs to NT36532 touch. DP remains active at 10 bpc / 30 bpp after the
query, and the saved kernel interval contains only audit-related messages.
No Chrome report or GL pixel regression is captured. A prior release-9 display
fault remains unresolved; shared-display codec workloads and lifecycle tests
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
not advertised. The installed revision 0.4 remains the default-query baseline.

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
