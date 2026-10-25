# Uke stateful video integration

Date: 25 October 2026. Target: POCO Pad X1 / Xiaomi Pad 7, SM7675.

The physical Fedora 46 tablet runs Linux 7.3.0-3. Its installed `qcom-iris`
module has no Cliffs/SM7675 match. The OEM `qcom,cliffs-vidc` node is unbound;
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
FastCV/CDSP VPP is disabled. Main10 encode remains implementation work; it is
not advertised. AV1 has no admitted Uke source/runtime evidence.

Build requires the original distribution Meson 1.12.1-2.fc46, which uses
Python on the build host. Meson is the upstream build system; replacing it
would create an unnecessary parallel build recipe. No Python file, build
tool or Python runtime is included in the video RPM.

Remaining work: complete the Linux 7.3 VideoCC/domain/reset/firmware/IOMMU
platform port, validate native H264/HEVC/VP9 sessions, adapt the four upstream
VA kernel deltas, implement real Main10 encode, and execute the codec,
lifecycle, performance and application matrix from the 25 October research
report. Firmware stays private and is not included in the COPR source RPM.
