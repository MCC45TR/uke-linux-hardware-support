# Standard s2idle policy — 18 October 2026

Environment: POCO Pad X1, Fedora Rawhide AArch64, Linux7.2.11-2 and systemd262.
The running kernel enables CONFIG_SUSPEND and advertises s2idle/deep, selecting
deep initially. Local laboratory masks block sleep.target and suspend.target;
no suspend/resume result exists in the recorded kernel statistics.

At the user's explicit request, adaptive-display10 supplies the standard
systemd sleep.conf drop-in selecting `SuspendState=mem` and
`MemorySleepMode=s2idle`, with suspend allowed. A standard tmpfiles write selects
s2idle at startup/package installation. No new resident process, suspend hook,
private sleep API or hardware register value is introduced. The current
PowerDevil button preferences remain separate. Systemd256 or newer is required.

On an existing installation, an administrator must deliberately remove earlier
laboratory masks with `systemctl unmask sleep.target suspend.target`. RPM
scriptlets do not override administrator masks. Fresh distribution units need
no such migration. Hibernation is outside this policy and remains disabled.

This is a userspace selection of existing upstream kernel/systemd interfaces;
no donor suspend driver or board values are required or copied. The kernel's
advertisement is not physical low-power/wakeup qualification. First validate
the RTC alarm and power-key wake path, then perform a bounded s2idle trial with
original KDE, display/touch, sensors, network and charging observations. Record
RTC/PM statistics and hardware results separately. A timed device-test phase
does not establish actual suspend entry or energy savings.

References: Linux `Documentation/admin-guide/pm/sleep-states.rst` and original
[systemd-sleep.conf](https://www.freedesktop.org/software/systemd/man/latest/systemd-sleep.conf.html).
