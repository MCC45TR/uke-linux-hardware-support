# Uke stateful video integration

Date: 25 October 2026. Target: POCO Pad X1 / Xiaomi Pad 7, SM7675.

The latest approved tablet query uses Fedora and Linux 7.3.0-10. Explicit
V4L2 selection initializes libva successfully; default selection fails because
the MSM driver alias is absent. No codec workload is accepted. A prior
release-9 display fault remains unexplained; the controlled release-10
provider-only trial does not repeat it. Active-display coexistence and codec
output remain separate acceptance gates. Iris uses video1/video2 in this
window; video0 belongs to NT36532 touch. Further device experiments require
the Juo coordinator's exact-command/window approval.

The experimental package pins Radxa libva-v4l2 at
`6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41`. It intersects the implemented
profiles with V4L2 coded formats, raw formats and profile menus. This is API
availability, not decoded-frame or encoded-bitstream acceptance.

Select the backend explicitly during bring-up:

```sh
LIBVA_DRIVER_NAME=v4l2 vainfo --display drm --device /dev/dri/renderD128
```

Installed revision 0.2 omits the automatic MSM alias. Source revision 0.4
retains the upstream alias so default discovery can be tested without an
environment override. It is reserved for a separate test repository and is not
installed at this checkpoint. Normal channel promotion still requires native
codec and application acceptance.
FastCV/CDSP VPP is disabled. Revision 0.1 does not advertise Main10 encode.
Signed revision 0.2 is installed through DNF. It adds P010 input and matching
HEVC Main10 parameters;
it advertises that profile only when the kernel exposes both P010 and the
Main10 profile menu. Native bitstream, precision and GPU/import validation
remain open. AV1 has no admitted Uke source/runtime evidence.

Source revision 0.3 replaces the fixed 240-fps and 245-Mbit/s encode checks
with runtime bitrate-control bounds and returned Q16 frame-rate validation.
Host checks cover 480-fps requests, fractional rates and driver refusal or
substitution. Native AArch64 COPR 11205969 and its build fixtures pass; the
signed package is inspected but not installed. These checks do not establish
tablet throughput. The fixed VA dimension
bounds and the full native/application test matrix remain open.

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
