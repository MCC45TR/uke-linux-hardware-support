# Uke CPU power profiles

17 October 2026. Target: Fedora Rawhide AArch64, original KDE, Uke with
`qcom-cpufreq-hw` policies 0, 3 and 7. The installed PPD 0.30 currently reports
the placeholder platform driver. CPU frequency policies already exist.

The candidate adds a C++ CPU driver to upstream Power Profiles daemon 0.30.
It requires the exact Uke identity and all three kernel governors. Power saver
selects `powersave`, balanced selects `schedutil`, and performance selects
`performance`. Each policy is read back; a failed change restores previous
governors and returns an error. Firmware OPPs, frequency limits, boost and
thermal protection are retained. GPU profiles are not implemented.

Power saver holds CPU requests at the lowest allowed frequencies and can reduce
responsiveness. These governor choices need measured device acceptance; they
do not establish battery-life or thermal results. No invented platform-profile
sysfs interface or fake PPD driver is used.

The original Fedora PPD package supplies its D-Bus and Polkit policy. A separate
native-only package supplies the daemon candidate and service override. Removing
the package and restarting `power-profiles-daemon` restores the original daemon.
No KDE application is rebuilt. No Python code or runtime is in this package.

Source: upstream `power-profiles-daemon` 0.30, archive SHA-256
`90f2c95024861d9f4062abedd514084939224e7a49edd468f001af7260ec8e6c`.
Meson and its Python interpreter run only in the host build SDK; Meson is the
upstream build system and has no practical replacement here. Upstream CLI,
Python tests and completion generators are excluded from the candidate payload.
