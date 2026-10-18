# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). The imported components live in this repository as ordinary
folders, with their own sources and packaging rules. New native utilities are
maintained alongside these imported components.

## Status — 18 October 2026

On 18 October, POCO Pad X1 runs Fedora Rawhide AArch64 and original KDE.
Signed COPR kernel 7.2.10-27 installs through DNF and reaches KDE after a normal
reboot. The powered Juo dock enumerates USB hubs, its HID receiver and RTL8153
Ethernet function. Evdev records mouse motion and the user confirms operation.
Ethernet networking and HDMI output remain unqualified.

After the kernel-28 compressed firmware-loader change, the RTL8153 driver
reports firmware `rtl8153b-2 v2 04/27/23` through ethtool. Carrier is zero;
no Ethernet cable or traffic result is claimed.

Kernel 28 from signed COPR 11168191 also installs through DNF and returns to SSH
and the original KDE session after a normal reboot. The touch service is active
with zero restarts; the user confirms physical display and touch operation. Application 10
and charging helper 4 are installed from signed COPR packages. Metadata-only
charge status and explicit diagnostic refusals pass on kernel 28; the matched
kernel-29 firmware query remains pending. Adaptive-display 9 retains successful
original libddcutil initialization after reboot while excluding internal GENI
buses. External DDC/CI brightness remains untested. The half-RAM swap, balanced
profile and CDC disabled state persist. No KDE application is forked or rebuilt.

The battery's 100% display and Charging state do not establish termination or an
electrical fault. Manual PD disconnection is verified as Discharging at 28 degrees
Celsius. The installed profile refuses charge-control requests; battery bypass
and autonomous protection are not qualified. The new host boundary fixtures
validate software refusal, not physical charge cutoff. HDMI testing remains
behind the unresolved battery power-path gate. PMIC warnings, optional front-
color startup, white-flash behavior and complete stability remain open.

The user accepted VRR and touch on the retained development profile; that
acceptance applies to the tested profile. Earlier Linux `7.2.10-8` tests restore the original
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
