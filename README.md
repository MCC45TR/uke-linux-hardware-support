# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). The imported components live in this repository as ordinary
folders, with their own sources and packaging rules. New native utilities are
maintained alongside these imported components.

## Status — 26 October 2026

The POCO Pad X1 development installation uses Fedora Rawhide AArch64, original
KDE and signed Linux `7.3.0-15`. Normal boot, matching modules, boot-image
readback and the existing boot-health checks pass. Exported images and other
device variants have separate acceptance gates.

Recorded working functions include normal touch, Wi-Fi, Bluetooth mouse and
Space Q45 stereo playback, four-pose rotation, dual ambient-light response,
proximity and cover switch events. HDMI works on Linux 7.3, confirmed on
26 October. Earlier Juo tests establish native/Chromium monitor sound, dynamic
ELD monitor labels and Gigabit Ethernet connectivity. Later RX drops,
reconnection, cover transitions and sustained throughput remain open.

The signed KDE Connect policy enables local-network discovery; POCO F5 was
discovered on 24 October. Pairing and transfers remain untested. Original KDE
applications and normal user/admin policy remain authoritative. Chromium is
a removable recommendation rather than a hard dependency of the KDE selection.

Installed Control 1.14 remains separate from the expanded 2.3 CLI and new
charging/NFC source. Signed Control 1.12.0-5 and Linux 7.3.0-5 pass one visible
ten-second RGB color test and caller-refusal checks. Torch extension source,
real battery/camera LED policy and electrical/thermal/endurance work remain open.
No active Linux NFC reader or qualified charging-protection result is claimed.

On kernel 15 with signed VA 0.4, seven decode modes each produce 30 exact-reference 320x240
frames: H.264 Baseline/Main/High, HEVC Main/Main10 and VP9 Profile0/2, including
independent ten-bit references. The native FFmpeg High/B-frame path still fails.
Basic native H.264 Main encoding produces 30 independently decoded frames with
correct SPS timing; missing color metadata, rate control, larger modes and
endurance remain open. Browser video, zero-copy presentation, sandbox/GL
coexistence and cold-boot activation remain unqualified. Signed VA revision 0.6
preserves damaged-picture errors in source fixtures; it is not installed.

VRR remains disabled after input/display regressions. Internal audio, cameras,
sleep, charging protection, gesture wake, DDC/CI, HDR and experimental external
DSC/FEC retain their separate limits. The [hardware table](https://github.com/MCC45TR/uke-linux#hardware-support)
is the current function-level summary. Source, host, package and tablet results
are not interchangeable.

The [source checkpoint](https://github.com/MCC45TR/uke-linux-hardware-support/tree/publish/source-snapshot-20261026)
preserves native identity and graphics/video diagnostics. Identity fixtures and
Vulkan/GLES host builds pass; OpenCL probe compilation lacks host headers.
Local touch references and calibration remain private and device-specific.
The [video/browser checkpoint](https://github.com/MCC45TR/uke-linux-hardware-support/tree/publish/video-source-20261026)
retains the later bounded codec results and uninstalled browser/backend candidates.
The [Control packaging correction](https://github.com/MCC45TR/uke-linux-hardware-support/tree/fix/offline-core15-post-20261026)
passes native AArch64 COPR 11209527 as `1.15.0-2`; no tablet update is performed here.

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

The package notes below retain the earlier 18–19 October baseline; the current
tested cohorts and open limits are summarized above.

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
