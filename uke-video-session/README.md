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

The earlier launcher 1.0.0-3 measured 70.48 compositor submissions/s in a
180-second accelerated 4K60 trial and 59.94/s in a 300-second normal trial.
Those results do not qualify this automatic implementation. COPR, installed
normal-Chrome behavior, per-user activation, recovery, optical cadence and HDR
remain separate validation gates.

The implementation uses the standard
[D-Bus activation contract](https://dbus.freedesktop.org/doc/dbus-specification.html),
[Power Profiles API](https://upower.pages.freedesktop.org/power-profiles-daemon/gdbus-org.freedesktop.UPower.PowerProfiles.html)
and Linux thread affinity. All native compilation and fixtures run in COPR
for `fedora-rawhide-aarch64`; the runtime payload is C++.
