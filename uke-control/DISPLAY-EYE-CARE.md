# O82 eye-care and original GNOME integration

30 October 2026. Source review for Xiaomi Pad 7 / POCO Pad X1 (Uke), O82 panel,
SM7675 and the existing paired KTZ8866 backlight. No mode, register, brightness,
color setting, installed package or physical display was changed by this review.

## Separate capabilities

Xiaomi's [official product page](https://www.mi.com/global/product/xiaomi-pad-7/)
advertises DC dimming, hardware low-blue-light certification and flicker-free
certification. These product claims do not establish the current native Linux
waveform, a user-switchable panel mode or certification of a modified pipeline.
The [specification](https://www.mi.com/global/product/xiaomi-pad-7/specs/) also
lists a flicker sensor; that is not itself a panel anti-flicker actuator.

| Capability | Evidence and current boundary |
|---|---|
| Backlight dimming | Exact OEM display DT uses an external backlight. OEM KTZ8866 code remaps brightness and writes the brightness registers of both chips. The native paired owner already preserves that curve; no second writer is needed. |
| Panel dimming/CABC | OEM O81/O82 common DT contains distinct CABC and dimming command groups. Native O82 already sends the retained dimming group after the first frame. This alone does not measure PWM or prove a separate DC-dimming switch. |
| Backlight mode observation | Existing `uke_dimming_state` observes the two bound chips' configuration under their brightness lock. It reports read failures/mismatch and does not change mode. I2C brightness control is not proof of zero optical modulation. |
| Hardware low blue light | Advertised panel characteristic. No independently controllable low-blue-light command was established in the reviewed exact O82 common DT/backlight path. An OEM feature name is insufficient register evidence. |
| GNOME Night Light | Original GNOME color-temperature policy, delivered through Mutter's display/color pipeline. Distinct from panel optical certification. The existing native DPU path has DSPP color blocks and CTM/gamma resource handling. |

Exact Uke references:
[display DT](https://github.com/MiCode/vendor_qcom_proprietary_display-devicetree/tree/4a3e1c391130237c5e8c2b250d1c94f3e25b9b11/display),
[O82 common commands](https://github.com/MiCode/vendor_qcom_proprietary_display-devicetree/blob/4a3e1c391130237c5e8c2b250d1c94f3e25b9b11/display/dsi-panel-o81-o82-dualdsi-dsc-lcd-video-micommon.dtsi)
and [KTZ8866 source](https://github.com/MiCode/vendor_opensource_display-drivers/blob/44275c404a677d9d9e5230fedd1af2ef02fc7b9a/msm/mi_disp/mi_backlight_ktz8866.c).
Upstream [DPU color handling](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/msm/disp/dpu1/dpu_crtc.c)
and [DSPP implementation](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/msm/disp/dpu1/dpu_hw_dspp.c)
are the relevant standard hardware programming path; no foreign panel commands
or backlight mode bits are transplanted.

## GNOME session support

Use the distribution's original **Settings → Displays → Night Light**, as
described in [GNOME Help](https://help.gnome.org/gnome-help/display-night-light.html).
Control adds bounded read-only integration with the existing session services:

- `org.gnome.Mutter.DisplayConfig`, `/org/gnome/Mutter/DisplayConfig`:
  `NightLightSupported` (boolean compositor capability).
- `org.gnome.SettingsDaemon.Color`, `/org/gnome/SettingsDaemon/Color`:
  `NightLightActive`, `Temperature` (Kelvin), `DisabledUntilTomorrow`.

`uke-control --json desktop status` includes these fields. It pins service
owners, disables activation, preserves missing/wrong-type values as unknown and
does not impersonate a GNOME session from KDE/root. No new daemon, root helper,
GNOME fork, fake capability flag or forced user preference is introduced.
The interface contracts come from [Mutter](https://github.com/GNOME/mutter/blob/main/data/dbus-interfaces/org.gnome.Mutter.DisplayConfig.xml)
and [Settings Daemon](https://github.com/GNOME/gnome-settings-daemon/blob/master/plugins/color/gsd-color-manager.c).

Private-bus fixtures validate the reader, including supported=false and missing
temperature/service handling. They cannot establish GNOME's actual appearance,
hardware versus shader execution, output color accuracy or a panel low-blue mode.
Those require an original GNOME session on a matched kernel, per-output color
checks and separate attended optical measurements. Optical flicker/DC-dimming
acceptance needs measured waveforms across brightness and refresh rates; a
camera image or a configuration bit alone cannot close it.
