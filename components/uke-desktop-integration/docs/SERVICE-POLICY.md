# Desktop service policy — 19 October 2026

Environment: POCO Pad X1, Fedora Rawhide AArch64 and Linux 7.2.11-5. The desktop
continues to use original Fedora applications. Source defaults, installed device
settings and measured behavior are separate evidence.

## Software RAID

Release 11 supplies vendor presets that disable mdmonitor.service and
raid-check.timer by default. The tested tablet has ordinary UFS partitions,
no md device and no mdadm configuration. Existing installations retain their
administrator service choices; an RPM upgrade does not stop an active array.
RAID users can explicitly enable the original distribution units. Scheduled
fstrim and normal disk/USB management are retained.

The tablet's unused RAID units were explicitly disabled after those checks.
This removes an unused weekly job; it does not establish a large CPU saving or
a shorter desktop startup. Sensor, authentication, network, clock, thermal and
firmware-update services are not classified as unnecessary by their names.

## Firmware daemon idle behavior

Original fwupd 2.1.8 takes about 4.8 seconds during the measured first startup.
Its upstream default inhibits idle shutdown when startup exceeds 500 ms. The
idle process remains allocated at about 55 MiB. A controlled 60-second timeout
trial restores automatic exit; normal D-Bus activation remains available.

For the device policy, retain the upstream five-minute IdleTimeout and merge
the following key into the existing administrator configuration:

```ini
[fwupd]
IdleInhibitStartupThreshold=0
```

Zero disables only the startup-duration inhibitor. Device/plugin and active
firmware-operation inhibitors remain in force. Do not overwrite other settings
or credentials. The package does not modify fwupd's configuration, replace its
daemon, disable plugins or install firmware. The original client's ModifyConfig
interface does not admit this key; use the documented administrator file.

Source: fwupd 2.1.8, commit
`3b7eca282bbe6cb4e8d5b4e42f984d2a1679977f`,
[configuration documentation](https://github.com/fwupd/fwupd/blob/3b7eca282bbe6cb4e8d5b4e42f984d2a1679977f/docs/fwupd.conf.md),
src/fu-daemon.c and libfwupdplugin/fu-config.c. A warm screen-off restart takes
about nine seconds, so a narrow ten-second startup threshold is not a robust
device policy. Idle exit trades cached readiness for later on-demand startup;
this is not a measured battery-life improvement.

## Release limits

Release 12 adds an administrator rule with the original Fedora
60-block-scheduler.rules filename. It skips the generic BFQ assignment only
when the block device has a ufshcd-qcom ancestor. Linux 7.2.11 selects
mq-deadline for the tested single-queue/shared-tag UFS LUNs; simultaneous
runtime switches can return EBUSY and leave inconsistent LUN schedulers.
The remaining Fedora disk rule is retained for external storage.

The configuration is installed with noreplace semantics. It does not force a
live scheduler switch, trigger devices, change queue depths or add a daemon.
Administrator overrides remain available. Recompare this small override when
the original distribution rule changes; its current source SHA-256 is
05cda1baec9b869bbc6371b75462aa5e2cff2165e49778f19955dcb3bf09b54e.
Rule syntax and package checks are separate from post-reboot UFS acceptance.

Screen-off CPU, active desktop work and package updates require distinct
measurements. A post-boot Discover update is not an idle benchmark. The system
manager's startup duration excludes bootloader and physical first-frame time.
Do not move required work past a target just to shorten its reported timestamp.
Keep logs, SELinux, filesystem ordering, recovery routes and hardware protection.
