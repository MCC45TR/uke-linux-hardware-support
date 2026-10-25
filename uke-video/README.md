# Uke stateful video integration

Date: 25 October 2026. Target: POCO Pad X1 / Xiaomi Pad 7, SM7675.

The last video-owned tablet observation uses Fedora 46 and Linux 7.3.0-9.
It enables the missing video configuration gates, but a late VIDEOCC-only
load is followed by display translation faults before Iris loads. Iris later
registers a decoder and encoder; no codec workload is accepted. Further device
experiments require the Juo coordinator's exact-command/window approval.
`/dev/video0` belongs to NT36532 touch. No native codec session is accepted.

The experimental package pins Radxa libva-v4l2 at
`6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41`. It intersects the implemented
profiles with V4L2 coded formats, raw formats and profile menus. This is API
availability, not decoded-frame or encoded-bitstream acceptance.

Select the backend explicitly during bring-up:

```sh
LIBVA_DRIVER_NAME=v4l2 vainfo --display drm --device /dev/dri/renderD128
```

The automatic MSM alias is held until native V4L2 and application validation.
FastCV/CDSP VPP is disabled. Revision 0.1 does not advertise Main10 encode.
Signed revision 0.2 is installed through DNF. It adds P010 input and matching
HEVC Main10 parameters;
it advertises that profile only when the kernel exposes both P010 and the
Main10 profile menu. Native bitstream, precision and GPU/import validation
remain open. AV1 has no admitted Uke source/runtime evidence.

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

Remaining work: complete native power and firmware startup, validate
H264/HEVC/VP9 sessions and end-to-end Main10 encode, and execute the
codec, lifecycle, performance and application matrix from the research report.
