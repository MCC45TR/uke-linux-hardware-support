# Uke video session policy

30 October 2026. Fedora Rawhide AArch64 integration for ordinary Chrome
sessions. The native VA backend reports actual UHD decode activity to a
D-Bus-activated service in the current user's session. It does not launch or
replace Chrome, select a profile, inspect URLs or require a special shortcut.
Each user receives the same activation files through the RPM.

Fresh UHD activity from an authenticated original Chrome GPU process requests
a standard performance hold and a capacity-based CPU preference for that
process. Pausing, closing or losing the decoder expires activity and restores
the original affinity. A private runtime checkpoint supports recovery after
service failure. Manual affinity/profile changes and power-saving selection
take priority. Hardware clocks, thermal limits and the browser sandbox remain
under their existing owners.

The service starts only when requested and exits after inactivity. It runs as
the session user, with no root daemon, login-time polling or browser extension.
The native protocol contains a context counter, dimensions and activity state;
it carries no media, website, account or profile data.

COPR 11230049 builds service 1.1.0-1; 11230057 builds the matching backend.
Signed DNF delivery on POCO Pad X1 passes ordinary-Chrome activation,
pause/resume, manual-profile priority and forced-crash recovery controls.
Separate-UID activation and caller rejection pass independently of playback.
A 300-second ordinary-Chrome trial retains 3840x2160 in all 601 polls,
measures 59.93 compositor submissions/s and reports 41 drops, with no waiting,
error or resolution fallback. A separate 180-second trial at 1.25x speed
measures 70.72 submissions/s and 895 reported drops at the 75 fps target,
without waiting or quality fallback. This is processing headroom, not frame
generation. Optical cadence and HDR remain unqualified.

The implementation uses the standard
[D-Bus activation contract](https://dbus.freedesktop.org/doc/dbus-specification.html),
[Power Profiles API](https://upower.pages.freedesktop.org/power-profiles-daemon/gdbus-org.freedesktop.UPower.PowerProfiles.html)
and Linux thread affinity. All native compilation and fixtures run in COPR
for `fedora-rawhide-aarch64`; the runtime payload is C++.
