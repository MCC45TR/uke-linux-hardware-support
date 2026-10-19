# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). The imported components live in this repository as ordinary
folders, with their own sources and packaging rules. New native utilities are
maintained alongside these imported components.

## Status — 19 October 2026

On 18 October, POCO Pad X1 runs Fedora Rawhide AArch64 and original KDE.
Signed COPR kernel 7.2.10-27 installs through DNF and reaches KDE after a normal
reboot. The powered Juo dock enumerates USB hubs, its HID receiver and RTL8153
Ethernet function. Evdev records mouse motion and the user confirms operation.
Ethernet networking and HDMI output remain unqualified.

After the kernel-28 compressed firmware-loader change, the RTL8153 driver
reports firmware `rtl8153b-2 v2 04/27/23` through ethtool. Carrier is zero;
no Ethernet cable or traffic result is claimed.

Kernel 29 from signed COPR 11168479 installs through DNF and returns to SSH and
original KDE after a normal reboot. The user confirms display/touch; VRR Always
persists without restoration, with about 30 Hz measured idle DPU at the 144 Hz
upper mode. Application 11 and runtime/helper 6 are installed from signed COPR
11168593/11168630. Explicit threshold GETs report disabled 70/80. The restricted
enable operation refuses the old-kernel write interface without fallback.
Charge workers remain inactive. Adaptive-display 9 retains original libddcutil
initialization, excluding internal GENI buses. External DDC brightness is untested.
Half-RAM swap, balanced profile and CDC disabled state persist.

The separate minimal native offline UI is installed. It draws a large battery,
percentage, firmware time-to-full, signed battery power and temperature; missing
time remains unavailable. Normal-boot refusal passes without interrupting KDE
or touch. Its matched initramfs targets Linux 7.2.11, currently in full COPR.
Physical charger boot, key/unplug behavior and efficiency remain untested. See
[the interface contract](uke-platform-runtime/docs/OFFLINE-CHARGING.md).
No KDE application is forked or rebuilt.

The battery's 100% display and Charging state do not establish termination or an
electrical fault. Manual PD disconnection is verified as Discharging at 28 degrees
Celsius. The installed profile refuses charge-control requests; battery bypass
and autonomous protection are not qualified. The new host boundary fixtures
validate software refusal, not physical charge cutoff. HDMI testing remains
behind the unresolved battery power-path gate. PMIC warnings, optional front-
color startup, white-flash behavior and complete stability remain open.

19 October 2026, Linux 7.2.11-6: touchscreen release 10 stalls in contact
assignment and reaches a 6 GiB memory peak. Signed COPR release 11 is installed
through DNF; a 30-second physical scrolling trial reports uninterrupted input
with about 9 MiB RSS. Separate short firmware watchdog recoveries remain open.
Ten-finger input later causes transient display/touch corruption with Always
VRR. Changing only VRR to Never at the same 60 Hz base removes the reported
symptom; VRR remains disabled for this control. Release 12 adds OEM charger-noise
policy restoration, bounded configuration input and tracking-ID wrap. Host
ASan/LSan checks, 18800 private-packet replay operations and 100 power-monitor
lifetimes report no memory/descriptor failure; these are host results. Signed
device delivery and full lifecycle qualification are recorded in
[the firmware audit](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/testing/UKE-TOUCH-FIRMWARE-AUDIT-2026-10-19.md).
[The diagnosis and limits](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/lessons/2026-10-19-UKE-TOUCH-ASSIGNMENT.md)
supersede unrestricted long-duration touch acceptance. The ten-contact fault
reopens earlier VRR acceptance. Earlier Linux `7.2.10-8` tests restore the original
KDE sensor interfaces. Previous
.4056 normal-boot user tests confirm four-pose rotation and dual-light
automatic brightness. Bluetooth 5.2 mouse movement, clicks and
wheel input pass physical tests. Native CPU/GPU profiles operate through PPD;
touchscreen readiness follows actual input publication. Signed COPR packages
are installed through DNF. Factory firmware and calibration remain separate,
device-local inputs. The downloadable Alpha 1 image is not accepted by these
results.

Physical proximity far/near/far and Hall lid/fold transitions pass. The original
KDE sensor backend reads all 27 configured PMIC channels with standard units.
The internal charger-temperature channel reads 31.29–31.41 °C with USB power;
without USB power its near-zero input produces the OEM-clamped 0 °C. Its absolute
accuracy is unqualified. The compass returns changing headings; absolute
heading accuracy, full thermal policy and lifecycle endurance remain incomplete. Full shutdown
can hang in the original KDE clipboard worker. See the
[hardware table](https://github.com/MCC45TR/uke-linux#hardware-support) for the
exact tested functions and limits. Package success alone does not establish
hardware support.

Original Fedora Plasma Login Manager starts the session. Declarative udev
policy keeps rear-cover state and ordinary mice from overriding KWin's tablet
attachment heuristic. No pogo keyboard is available; native pogo input,
authentication and illumination remain unimplemented and untested. No keyboard
light control is fabricated without hardware.

## Components

| Folder | Purpose |
|---|---|
| [uke-core-meta](uke-core-meta) | Console package selection |
| [uke-desktop-metas](uke-desktop-metas) | Desktop package policy |
| [uke-desktop-packaging](uke-desktop-packaging) | Native libraries, theme and decoration plugin |
| [uke-orangefox-packaging](uke-orangefox-packaging) | Versioned recovery images managed by DNF |
| [uke-boot](uke-boot) | Boot and USB debug integration |
| [uke-platform-runtime](uke-platform-runtime) | Native platform runtime |
| [uke-sensors](uke-sensors) | Sensor discovery and native tools |
| [uke-control](uke-control) | Native device settings, swap and development diagnostics |
| [uke-camera](uke-camera) | Camera integration work |
| [xiaomi-uke-firmware](xiaomi-uke-firmware) | Firmware sources, licenses and file identities |
| [uke-desktop-integration](uke-desktop-integration) | Desktop integration |
| [plasma-uke-kcm](plasma-uke-kcm) | Planned device settings panel |
| [uke-hardware-provenance](uke-hardware-provenance) | Hardware source records |

KDE applications come from the distribution. They are not cloned, forked or
rebuilt as Uke applications. Unmodified original Fedora desktop packages may retain their distribution
Python dependencies under the documented exception. Project tools, recovery
and initramfs do not ship project Python or a Python runtime.

Signed COPR application 8, battery runtime 3, Bluetooth integration 6 and boot
integration 20 are installed through DNF. Application self-checks and the
one-shot manual CDC-off command pass. On the release-26 boot, the saved policy
is disabled, g_serial is absent and the legacy shell service remains masked.
Half-RAM swap (3690 MiB) and the first
partition inventory survive a normal reboot; custom, full-RAM and half-RAM file
sizes pass. Existing swap-partition preference remains untested. The native C++
backend supports future independent frontends. Development diagnostics are
selected explicitly and excluded from release image package lists.

The opt-in `uke-front-color 0.1.0-2` package publishes raw CCT sensor snapshots
at `/run/uke-sensors/front-color.json`. On kernel 23 with enforcing SELinux,
unprivileged reads, stale-sample reporting, stop/restart and crash cleanup pass.
Units, calibration and DSP clock mapping remain unverified. The installed
Bluetooth helper verifies the configured controller address against the
protected read-only factory record. Both factory-address startup services exit
successfully on release 26; persist remains read-only.
LDAC playback with Soundcore Space Q45 was reported working; no codec trace is
recorded here.

Application 8 and boot integration 20 are installed packages for a read-only
front-color command, hardened factory WLAN reads and explicit one-shot CDC.
CDC defaults to off; the new manual API saves no startup policy and requires no
watcher service. A matched full Type-C kernel/root pair is required. Docked
host, PD, SuperSpeed, external display and charging-control acceptance remain
open. Battery admission recognizes the driver and refuses unqualified control;
an active service or metadata response does not establish an applied charge cap.

Application 8 selects the actual internal DSI connector and active CRTC when
DP resources are also present. Its signed COPR 11167901 build, native checks,
DNF installation and ordinary-user vblank observation pass on kernel 26. This
is DPU timing evidence, not optical panel precision or cadence measurement.
In the first powered-dock trial, the battery reports charging without a UPower
warning, but Type-C partner and USB dongle enumeration remain absent. Adding
HDMI produces no detected sink and the monitor shows no tablet image. USB host
and external display operation are not accepted by these results.

## Checks and builds

```sh
make validate
make test-native
```

These commands check component rules and existing native tests. They do not
test tablet hardware. [SOURCE-IMPORTS.json](SOURCE-IMPORTS.json) records the
original source imports and their Git history.

COPR builds configured package sources after GitHub changes. Shared-rule changes
request the affected builds through the root workflow. The daily stable workflow
checks Material Decoration and eligible recovery releases. Active workflows
live in [`.github/workflows/`](.github/workflows/).

The kernel, image builder, recovery device tree and UEFI port have separate
repositories linked from [Uke Linux](https://github.com/MCC45TR/uke-linux#project-repositories).
Detailed build readiness is in the [private package hub](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/PACKAGE-HUB.md).
