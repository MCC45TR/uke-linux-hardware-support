# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). All twelve components live in this repository as ordinary
folders, with their own sources and packaging rules.

## Status — 18 October 2026

On POCO Pad X1, Fedora Rawhide AArch64 and persistent Linux
`7.2.10-senemos.1.4056`, normal boot restores four-pose rotation and dual-light
automatic brightness in original KDE. Bluetooth 5.2 mouse movement, clicks and
wheel input pass physical tests. Native CPU/GPU profiles operate through PPD;
touchscreen readiness follows actual input publication. Signed COPR packages
are installed through DNF. Factory firmware and calibration remain separate,
device-local inputs. The downloadable Alpha 1 image is not accepted by these
results.

Compass accuracy, proximity transitions, hall mapping, remaining PMIC scalers,
full thermal policy and lifecycle endurance remain incomplete. Full shutdown
can hang in the original KDE clipboard worker. See the
[hardware table](https://github.com/MCC45TR/uke-linux#hardware-support) for the
exact tested functions and limits. Package success alone does not establish
hardware support.

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
| [uke-camera](uke-camera) | Camera integration work |
| [xiaomi-uke-firmware](xiaomi-uke-firmware) | Firmware sources, licenses and file identities |
| [uke-desktop-integration](uke-desktop-integration) | Desktop integration |
| [plasma-uke-kcm](plasma-uke-kcm) | Planned device settings panel |
| [uke-hardware-provenance](uke-hardware-provenance) | Hardware source records |

KDE applications come from the distribution. They are not cloned, forked or
rebuilt as Uke applications. Unmodified original Fedora desktop packages may retain their distribution
Python dependencies under the documented exception. Project tools, recovery
and initramfs do not ship project Python or a Python runtime.

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
