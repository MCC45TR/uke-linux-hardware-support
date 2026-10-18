# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). The imported components live in this repository as ordinary
folders, with their own sources and packaging rules. New native utilities are
maintained alongside these imported components.

## Status — 18 October 2026

On POCO Pad X1 with Fedora Rawhide AArch64, signed COPR Linux `7.2.10-8`
boots persistently and restores the original KDE sensor interfaces. Previous
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

On 18 October, signed COPR `uke-control 0.1.0-3` and its separate development
test package were installed on POCO Pad X1 with Fedora Rawhide AArch64 and Linux
`7.2.10-23`. Half-RAM swap (3690 MiB) and the initial partition inventory survive
a normal reboot. Custom, full-RAM and half-RAM size changes pass. The C++ backend
and CLI support future independent frontends; no GUI is included. Development
tests are selected explicitly and are absent from release image package lists.
Charge-policy previews report unavailable actuators; charge-limit and USB-role
control remain unqualified. Physical uninterrupted-touch and panel VRR acceptance
remain separate from these application results.

The next USB source candidate defaults CDC to off and adds explicit
`uke-control set cdc on|off`, USB topology observation and guarded Type-C
data-role requests. ACM opt-in does not enable a root shell or network sharing.
It requires a matched full Type-C kernel/root pair; the installed kernel 23
remains fixed peripheral. Docked host, PD, SuperSpeed and external-display
acceptance remain pending.

Signed COPR build 11167396 later installed `uke-control 0.1.0-4` and its test
subpackage through DNF. Package integrity and root/unprivileged USB inventory
pass on kernel 23. Manual CDC and role requests reject the missing full-profile
admission without changing the legacy gadget or saving a policy. The default-off
root support set from build 11167392 is staged for the matched kernel update.

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
