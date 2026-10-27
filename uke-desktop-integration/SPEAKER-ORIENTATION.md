# Internal speaker orientation

Date: 27 October 2026. Target: Xiaomi Pad 7 / POCO Pad X1, Uke. Source and host
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
It validates all four control names, types and angle labels before any
write, skips unchanged values and checks each write by reading it back. ALSA
and logind events trigger reconciliation; missing/failing controls use a bounded
retry interval. A missing control on older kernels cannot enable speakers.
The four control writes are sequential, not one hardware transaction; partial
failure is reported and retried. It never raises volume, enables a switch,
starts playback or changes DisplayPort/HDMI/Bluetooth routing.

Install `uke-speaker-orientation` with the matching test kernel and existing
`uke-displayport-audio` policy. Its XDG autostart entry covers original KDE and
GNOME applications in Wayland sessions; it does not implement X11 sessions.
No root daemon, target Python or desktop application fork is included.

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
