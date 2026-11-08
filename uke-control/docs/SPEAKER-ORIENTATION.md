# Internal speaker orientation

Date: 8 November 2026. Target: Xiaomi Pad 7 / POCO Pad X1, Uke. Source and host
fixtures only; physical channel order, live switching and desktop acceptance
remain unverified.

The reported working reference is landscape with USB-C on the right and the
left channel heard on the left. That operator report does not identify an exact
kernel build and is not a physical test of this candidate. The expected
quarter-turn sequence from that reference is USB-C right, bottom, left, top.

Kernel 7.3.0-23 adds `Speaker Rotation` enums, prefixed TopLeft, TopRight,
BottomLeft and BottomRight. Values are 0, 90, 180 and 270 degrees in the OEM
speaker-position reference. The driver changes only I2SCTRL channel bits 4:3,
caches requests while idle and restores the selection before unmute. Failed
live register access stops/resets the affected amplifier and leaves its cached
angle unchanged. Existing gain, preset, thermal, switch and stream gates apply.

| OEM angle | Speakers receiving left | Speakers receiving right |
|---|---|---|
| 0 | TopLeft, BottomLeft | TopRight, BottomRight |
| 90 | BottomLeft, BottomRight | TopLeft, TopRight |
| 180 | TopRight, BottomRight | TopLeft, BottomLeft |
| 270 | TopLeft, TopRight | BottomLeft, BottomRight |

The native C++ session helper uses standard Wayland `wl_output` version 4,
including `name`, committed `geometry` transform and current hardware mode.
Current KDE KWin and GNOME Mutter source both send these events. Only DSI-1
with the native 3200 x 2136 hardware mode is selected. External outputs,
ambiguous internal outputs and reflected transforms do not select a route.
Rotation lock and manual display changes follow the compositor's actual state;
accelerometer movement alone does not change audio routing.

The helper checks logind session identity, user, local Wayland type and active
state. User-manager autostarts may lack both a login cgroup and XDG_SESSION_ID;
the fallback accepts exactly one local Wayland session belonging to the user.
Multiple possible sessions refuse routing instead of following another desktop.
Version 0.2.0 prefers the additive `Speaker Display Rotation` card control.
The machine driver snapshots all four codec selections, verifies the result,
restores all endpoints on failure and invokes the existing amplifier fault
latch if restoration cannot be verified. It retains the DT endpoint geometry
and OEM position masks. The client validates the complete enum and confirms
the result; only an absent control selects compatibility with older kernels.
Versioned Uke card driver names are admitted alongside the original `sm7675`
driver, always with the exact `POCO-Pad-X1` card name.

The legacy path validates all four control names, types and angle labels before
any write, skips unchanged values and reads back each result. ALSA
and logind events trigger reconciliation; missing/failing controls use a bounded
retry interval. A missing control on older kernels cannot enable speakers.
Its four compatibility writes are sequential; partial failure is reported and
retried. The native card operation also does not promise simultaneous I2C
changes across amplifiers. It never raises volume, enables a switch,
starts playback or changes DisplayPort/HDMI/Bluetooth routing.

Control 2.10 owns this session layer in its existing main runtime RPM. It
replaces the separate orientation package through versioned Provides/Obsoletes
and retains the executable and configuration paths, including the existing
XDG autostart identity. The audio producer is retired only after the normal
DNF ownership/configuration transition and recovery are verified. Archived
DisplayPort policy and speaker-control inputs remain source references.
The notifier uses ALSA, Wayland and logind directly and adds no root service.
Its XDG autostart covers original KDE and
GNOME applications in Wayland sessions; it does not implement X11 sessions.
No root daemon, target Python or desktop application fork is included.
The compositor's actual internal-output transform remains session policy.
Accelerometer readings cannot replace it when rotation is locked, manually
selected or an external display remains active with the cover closed.

The default display-zero offset is 0 relative to the native output transform.
This retains the source reference; its alignment with the reported USB-C-right
position and all four orientations still needs testing on the candidate. If
that test establishes a fixed
mount/reference offset, `/etc/uke-speaker-orientation-zero` may contain exactly
one of `0`, `90`, `180` or `270`, followed by a newline. The helper adds that
offset modulo 360 at its next start. Malformed/unreadable configuration refuses
routing. This option does not supply amplifier calibration.

Host fixtures cover all sixteen position/angle combinations, preservation of
other register fields, idle/resume restoration, failed register access, complete
control validation, readback, partial retry, active-session refusal, atomic
Wayland output updates and external/ambiguous/removed outputs. These checks do
not produce sound or establish either compositor's physical operation on Uke.

Remaining acceptance: establish the native display-zero reference, verify left
and right in each of four orientations, rotation lock/manual rotation, active
playback transitions, logout/session switch, suspend/resume, late audio-card
arrival and external-output independence. Use the existing qualified preset
and gain/protection limits; record exact kernel, firmware and desktop versions.
