# Display controls and source boundaries

18 October 2026. Source review for POCO Pad X1 / Xiaomi Pad 7, Uke O82 PB/42,
original Fedora KDE 6.8.0 and Linux 7.2.10. This record does not qualify every
panel supplier, external display adapter or system-sleep state.

## Original KDE configuration

PowerDevil tag v6.8.0, peeled commit
`d2e50173ebb1ee719e80932c67a1866400a8af71`, defines
`PowerButtonAction=128` as ToggleScreenOnOff. The profile schema is
`[AC|Battery|LowBattery][SuspendAndShutdown]` in powerdevilrc. Its handler
uses the existing DPMSControl ToggleOnOff action. The package supplies only
these defaults; it does not modify the upstream implementation.

Linux 7.2.10 `drivers/gpu/drm/msm/dp/dp_display.c` names its platform driver
`msm-dp-display`. `dp_aux.c` gives the DP device to the AUX implementation;
`drivers/gpu/drm/display/drm_dp_helper.c` makes that device the I2C adapter's
parent. The udev rule uses that driver ancestry, following ddcutil's active-seat
uaccess policy without granting sensor/touch buses access. Existing Fedora
DDC tools remain responsible for monitor discovery and brightness control.
No OEM pin, power setting or foreign board value is transferred.

## Color resolution and RGB range

[Xiaomi's FAQ](https://www.mi.com/my/support/faq/details/KA-537666/) advertises
12-bit / 68.7-billion-color output. The exact selected OEM display DT,
commit `4a3e1c391130237c5e8c2b250d1c94f3e25b9b11`, sets 30-bpp input and
10-bpc DSC in dsi-panel-o82-42-02-0b-dualdsi-dsc-lcd-video.dtsi and the shared
dsi-panel-o81-o82-dualdsi-dsc-lcd-video-qcomcommon.dtsi. The active native
driver retains that RGB101010 / DSC 1.1, 8 compressed-bpp contract. Compressed
8 bpp is not an 8-bpc color-depth claim. Physical native depth versus panel
FRC is not established by marketing or these transport fields.

KWin v6.8.0, commit `066d1e17c945d9a89fee645e10a2bbd77f2e93a6`, advertises
MaxBitsPerColor only when DRM provides max bpc. KScreen v6.8.0, commit
`57cfd0aed8dccbbcf4aa37329e3ba5855dbf1e71`, hides the selector if the minimum
and maximum supported limits are equal. Merely attaching an 8–16 range would
not implement a hardware depth switch. The current fixed 10-bpc DSI path
has no qualified selectable 8/12/16-bpc modes, so no such range is fabricated.

KWin exposes RGB range only for a connector with the functional Broadcast RGB
property. The internal DSI pipeline has no corresponding RGB quantization
control. External DP/HDMI choices depend on their actual driver and sink;
this package adds no cosmetic connector properties, synthetic EDID, HDR or
color-gamut claim.

## Remaining physical tests

Verify two short power-key presses and display return through the unchanged
desktop. Verify that unrelated I2C buses receive no new seat access. When the
external DP path and a DDC/CI monitor are available, check hotplug, user access,
VCP brightness read/write and unplug. A package build or installed ddcutil
cannot substitute for that external monitor test. Keep real s2idle/deep sleep
for the separate explicitly selected validation stage.
