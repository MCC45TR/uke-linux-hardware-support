# Uke video session

30 October 2026. Experimental Fedora Rawhide AArch64 launcher for the original
Google Chrome Default profile. It uses the installed Iris CPU-copy option,
keeps GPU-process threads within the higher-capacity allowed CPUs, and requests
the standard performance profile while that Chrome session exists. It adds no
browser build, frame interpolation, firmware or clock/thermal override.

Open **YouTube (Chrome)** after closing existing Chrome windows, or run
`uke-video-session https://www.youtube.com/watch?v=LXb3EKWsInQ`. An already
running Chrome process retains its existing environment; a forwarded window
does not establish an optimized session. The package does not close it.

The power hold belongs to this launcher's D-Bus connection. It is released
when the launcher exits, including abnormal termination. Manual power-profile
changes take priority. Missing capacity data preserves the allowed CPU mask;
an unavailable power hold leaves the selected profile intact. Only descendant
GPU processes from this launch receive scheduling hints.

Manual controls on POCO Pad X1 with kernel 7.3.0-54, Chrome 156, VA 1.0.0-8 and
Mesa 26.2.6-17 measured 70.9 compositor submissions/s during an explicit 1.25x
4K60 stress trial, without waiting or resolution fallback. Drops remain at the
75 fps target. This is processing headroom, not 70 distinct frames from normal
60fps content. Packaged-launcher, optical presentation and HDR acceptance
remain separate checks.

The implementation uses the standard
[power-profile hold API](https://upower.pages.freedesktop.org/power-profiles-daemon/gdbus-org.freedesktop.UPower.PowerProfiles.html)
and Linux CPU affinity within the calling user's existing limits. It is
compiled only in COPR for `fedora-rawhide-aarch64`; policy fixtures are build
controls, not tablet playback evidence.
