# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). The imported components live in this repository as ordinary
folders, with their own sources and packaging rules. New native utilities are
maintained alongside these imported components.

## Status — 18 October 2026

On 18 October, POCO Pad X1 runs Fedora Rawhide AArch64. The full Type-C kernels
24 and 25 boot and reach SSH, but fail internal display and touchscreen startup.
Kernel 25 fixes the missing L3B supplier, exposing a DP connector-state Oops.
Kernel 26 initializes that state and changes panel backlight ordering to address
the reported white flash; its host checks pass and its COPR build is running.
The accepted kernel 23 bundle remains available. VRR and touch behavior on the
retained development profile was accepted during tablet use; that acceptance
applies to the tested profile. Earlier Linux `7.2.10-8` tests restore the original
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

Signed COPR application 7, battery runtime 3, Bluetooth integration 6 and boot
integration 20 are installed through DNF. Application self-checks and the
one-shot manual CDC-off command pass; the new-boot CDC state remains pending.
Half-RAM swap (3690 MiB) and the first
partition inventory survive a normal reboot; custom, full-RAM and half-RAM file
sizes pass. Existing swap-partition preference remains untested. The native C++
backend supports future independent frontends. Development diagnostics are
selected explicitly and excluded from release image package lists.

The opt-in `uke-front-color 0.1.0-1` package publishes raw CCT sensor snapshots
at `/run/uke-sensors/front-color.json`. On kernel 23 with enforcing SELinux,
unprivileged reads, stale-sample reporting, stop/restart and crash cleanup pass.
Units, calibration and DSP clock mapping remain unverified. The installed
Bluetooth helper verifies the configured controller address against the
protected read-only factory record; its next-boot path remains a separate test.
LDAC playback with Soundcore Space Q45 was reported working; no codec trace is
recorded here.

Application 6 and boot integration 20 are source candidates for a read-only
front-color command, hardened factory WLAN reads and explicit one-shot CDC.
CDC defaults to off; the new manual API saves no startup policy and requires no
watcher service. A matched full Type-C kernel/root pair is required. Docked
host, PD, SuperSpeed, external display and charging-control acceptance remain
open. Battery admission recognizes the driver and refuses unqualified control;
an active service or metadata response does not establish an applied charge cap.

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
