# Uke Control

Native C++ settings backend and terminal client for Uke (SM7675). GTK4 and
Qt/Breeze clients can use the same backend in future; neither GUI is included.
The original distribution desktop remains unchanged.

## Status — 18 October 2026

Signed COPR build 11166924 installed core and development packages through DNF
on POCO Pad X1, Fedora Rawhide AArch64, Linux 7.2.10-23. The systemd service
created a 3690 MiB swap file with the standard SELinux swap label. Changes to
512 MiB, full RAM and half RAM passed. SensorProxy, processed sensor inputs,
DRM sampling and bounded proximity discovery ran on the tablet. Reboot
persistence and preference for an existing swap partition remain untested at
this checkpoint. These observations do not establish physical VRR or sensor
acceptance.
The charge-limit control is available only when the driver exports a writable
`charge_control_end_threshold`. Current Uke firmware controls are not invented
or accessed through raw battery property requests. USB status reads cached
sysfs state; role switching, PD and DisplayPort management remain unqualified.

## Commands

```sh
uke-control status
uke-control inventory
sudo uke-control inventory --refresh
sudo uke-control swap set half
sudo uke-control swap set full
sudo uke-control swap set 4096   # MiB
uke-control charge status
sudo uke-control charge limit 80 # requires a standard supported threshold
uke-control usb status
uke-control test display 60
uke-control test display 30 --csv
uke-control test cadence 144
uke-control test touch /dev/input/event5 60
uke-control test sensors 30
uke-control test discover all
uke-control test sample accel
sudo uke-control report
```

Use the actual input node returned by the system; the example node is not a
stable identifier. Touch observation requires an already accepted held contact;
release, tracking loss or `SYN_DROPPED` returns failure. It never grabs or creates
input. The display monitor distinguishes the selected mode from measured DPU
vblank cadence and the DRM VRR property. It references vblank interrupts and is
not optical panel metrology or a power-efficiency benchmark. Wayland presentation
feedback is compositor evidence. Sensor numbers are observations, not automatic
physical PASS records. SSC metadata and sample helpers reuse the MIT-licensed
project sensor implementation; its Qualcomm protocol attribution is retained.

Install `uke-control-tests` explicitly for development. Release image package
lists must exclude it. The core client and shared library have no Wayland or
KDE dependency. The test package adds a temporary terminal Hz-monitor launcher.
Its window can be placed above other windows using the original KDE controls.

## Swap and first boot

`uke-control.service` uses the standard userspace swap interface after local
filesystems are ready. The kernel is not patched to allocate files. A unique
partition tagged `PARTLABEL=linux_swap` or `LABEL=linux_swap` with a valid Linux
swap signature takes precedence over the configured file size. A missing,
ambiguous or invalid signature is never repaired by formatting a partition.
No partition table, Android data, boot slot, persist filesystem or existing
unmanaged swap is modified. Existing zram remains configured independently.

Without that partition, the default size is half the kernel-reported RAM,
rounded down to MiB. `full` and explicit 64..65536 MiB are also supported.
Automatic file creation currently supports ext4 and XFS. Fully written private
files live under `/var/lib/uke-control/swap`, with `swapfile_t` labels on SELinux
systems. Creation retains at least 2 GiB or 10% of filesystem capacity, whichever
is greater. New swap is activated before old managed swap is retired. If memory
pressure prevents retirement, the old area stays active and a warning is emitted.
Unmanaged areas are never disabled. An interrupted `.allocating` file requires
inspection; it is not silently reused. No hibernation/resume configuration is
implied by these settings.

The root-private `partitions-first-boot.json` records the first observed layout;
`partitions-current.json` is refreshed at startup. Both include existing and
missing `userdata`, `linux`, `linux_boot` and `linux_swap` names through the full
partition inventory; missing partitions are not created. The inventory complements
`uke-boot` status without replacing its boot-slot validation or updater. Settings
are root-only and serialized. There is no setuid binary or privileged GUI.

## Reports

`report` collects bounded current/previous journals, KDE user logs, dmesg, failed
units, package versions, mount/partition state, DRM state, memory/interrupt/pressure
counters and command exit statuses. A partial collector is recorded explicitly.
Bundles are root-private under `/var/lib/uke-control/reports`; raw logs may contain
identifiers and must not be committed or attached to public issues. Calibration,
credential files, process environments and filesystem contents are not collected.

`uke-control report github DIR OWNER/REPO` uses the original optional `gh` client
and its existing authenticated account to submit **only** the generated anonymous
summary. It never uploads raw bundles. No token is built into the application.
The operator supplies the problem and reproduction steps in the resulting issue.
Raw private-log upload is not implemented. Collection and submission are separate
explicit commands.

## Build

```sh
make
make check
make srpm
```

Build dependencies: C++20 compiler, libselinux, libdrm, libsystemd, Wayland client
and the original stable xdg-shell/presentation-time protocol XML. Generated
Wayland protocol glue remains upstream C. No project Python payload or runtime
is used. Native self-tests cover sizing, refusal, ownership, command argument
isolation, bounded collection, cadence scheduling and the inherited SSC parsers.
Tablet installation, reboot and physical tests are recorded separately.
