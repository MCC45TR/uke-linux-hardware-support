# Uke Control

18 October 2026 source follow-up: `sudo uke-control charge apply-cap80` is a
separate one-shot development request for the firmware's already enabled 70/80
band. It requires the matched helper and kernel interface, disconnected input
and fresh device checks. It does not acquire a general control lease, set current
or voltage, retry a request or qualify bypass. An opaque firmware reply and
unchanged settings leave the physical effect unknown. This client source alone
does not implement the helper, install the feature or establish a tablet result.

Native C++ settings backend and terminal client for Uke (SM7675). GTK4 and
Qt/Breeze clients can use the same backend in future; neither GUI is included.
The original distribution desktop remains unchanged.

## Status — 18 October 2026

Application 11 and its separate development diagnostics are installed from
signed COPR 11168593 on POCO Pad X1, Fedora Rawhide AArch64. The active internal
DSI CRTC is selected even with multiple connectors. With VRR enabled, the monitor
labels the configured 144 Hz mode as the upper limit and reports approximately
30 Hz measured idle DPU cadence. These are host timings, not optical metrology.

Kernel 7.2.10-29 completes a normal reboot; the user confirms display and touch.
KDE VRR Always, the 3690 MiB half-RAM swap, balanced profile and disabled CDC
persist. Earlier standard swap resizing passes; an existing swap partition
remains untested. The powered Juo receiver works on kernel 27; actual Ethernet
traffic, HDMI and monitor brightness are separate unqualified functions.

Signed COPR 11168630 installs runtime/helper 6 and a separate native offline
charging screen. Exact source/signature/payload checks, native self-checks and
RPM verification pass. The deliberate kernel-29 query reports disabled 70/80
thresholds. The restricted enable-cap80 operation refuses the missing old-kernel
interface; ordinary users are denied. No fallback, SET or charge worker starts.
Physical cutoff, persistent cap and bypass remain unqualified. The offline UI's
normal-boot refusal passes; its matched initramfs and physical screen lifecycle
remain pending. See [charging boundaries](CHARGING.md) and the
[offline interface](../uke-platform-runtime/docs/OFFLINE-CHARGING.md).

## Commands

Source release 12 adds [channel synchronization and updates](CHANNELS.md) and
the [live sensor view](SENSORS.md). These changes are prepared and tested on the
host; the installed application-11 observations above remain separate.

```sh
uke-control status
uke-control brunch status
sudo uke-control brunch stable --dry-run
sudo uke-control brunch test
sudo uke-control brunch unstable
sudo uke-control brunch sync
sudo uke-control update --check
sudo uke-control update
uke-control sensors -n5
uke-control sensors -n5 --count=3 --json
uke-control inventory
sudo uke-control inventory --refresh
sudo uke-control swap set half
sudo uke-control swap set full
sudo uke-control swap set 4096   # MiB
uke-control charge status
uke-control charge modes
uke-control charge plan charge80 # metadata only; no actuator
uke-control charge capabilities
uke-control charge protocols     # cached observations; exact protocol may remain unknown
sudo uke-control charge admission # metadata only; no firmware request
sudo uke-control charge settings # explicit firmware GETs; no permission or SET
sudo uke-control charge enable-cap80 # matched kernel; existing70/80 only, input off
sudo uke-control charge mode charge80 # requires qualified autonomous thresholds
sudo uke-control charge mode automatic # requires qualified per-boot admission
sudo uke-control charge limit 80 # same qualified native 80% request
uke-control usb status
uke-control usb watch 60
sudo uke-control usb request data host
sudo uke-control set cdc on
sudo uke-control set cdc off
uke-control test display 60
uke-control test display 30 --csv
uke-control test cadence 144
uke-control test touch /dev/input/event5 60
uke-control test sensors 30
uke-control test discover all
uke-control test sample accel
uke-control test sample cct_front # raw descriptor-bound fields; units unverified
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

Charging previews describe 80% cap, 80-90% autonomous hold, slow and automatic
profiles without applying them. Explicit requests use the separately installed
`uke-charge-control` helper and its per-boot qualification. The shipped backend
profile is unqualified. An active automatic worker is reported separately from
an applied current setting. See [charging policy limits](CHARGING.md).

The source command `uke-control display sensor` reads the optional native
front-color snapshot without starting collection. It exposes raw values with
unverified units and independently checks publisher/sample recency. See
[the display and front-color interface](DISPLAY.md) for the exact contract and
remaining coordinator/package/device validation. This is not calibrated Kelvin
or an automatic white-point setting.

## USB and CDC

`usb status` returns bounded JSON containing cached Type-C and controller roles,
UDC state, USB device/interface IDs and speeds, bound drivers, USB NIC counters
and external display modes. Missing values remain null. `usb watch` records one
snapshot per second for a bounded interval. Neither command reads power-supply
attributes, activates D-Bus services or collects serial strings, MAC address
attributes or EDID bytes. Interface names may still encode device identifiers;
snapshots belong in private reports. These observations do not prove PD contracts, coherent
attachment epochs, display depth or physical functionality.

`usb request data host|device` requires root, the matched `typec-v1` Uke profile
and the shared gadget ownership API. It requests the standard Type-C `data_role`
through the kernel's UCSI provider; it never writes `usb_role`, PHY registers or
charger properties. Attached USB storage, an active USB NIC or enabled external
display blocks a change. Only the recorded automatic ACM gadget may be torn down.
Other gadgets and legacy `g_serial` are refused. A changed partner aborts the
request. A successful sysfs write and a matching Type-C/controller readback are
reported separately; neither establishes physical acceptance. There is no
automatic retry or saved role override, and no power-source or lane-force control.

CDC defaults to off. With manual CDC API2, `set cdc on` invokes the existing
root helper once; a computer must already attach in device data role. A missing
or ambiguous attachment is refused without a background watcher or retry.
`set cdc off` removes only the recorded manual ACM gadget and refuses an old
automatic or foreign project gadget profile before disconnecting it. Neither
command starts/enables a service, saves a startup opt-in, unloads an inherited
serial owner or enables a root console, Ethernet gadget, hotspot or network
sharing. Binding is reported separately from physical PC enumeration. The
matched root20 capability is required; old support cannot silently substitute
its service-based behavior. Before upgrade, the coordinator must explicitly
stop/disable the old watcher and retain the disabled policy using the reviewed
root migration contract. No automatic migration is performed by this app.

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
