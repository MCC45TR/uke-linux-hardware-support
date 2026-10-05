# Uke packages

This directory groups the platform's package sources in the workspace and on
GitHub. Each entry is a pinned Git submodule with its own license, history and
COPR source URL. Grouping the checkout does not rename its upstream repository
or interrupt automatic builds.

| Source | Purpose |
|---|---|
| [uke-core-meta](uke-core-meta) | Reviewed console package selection |
| [uke-desktop-metas](uke-desktop-metas) | Desktop policy metadata; graphical admission remains blocked |
| [uke-desktop-packaging](uke-desktop-packaging) | Reviewed non-KDE native libraries, theme and requested decoration plugin |
| [uke-orangefox-packaging](uke-orangefox-packaging) | DNF-managed, versioned recovery image data |
| [uke-boot](uke-boot) | Future independent Uke boot integration |
| [uke-platform-runtime](uke-platform-runtime) | Future native platform runtime |
| [uke-sensors](uke-sensors) | Sensor source and hardware admission |
| [uke-camera](uke-camera) | Camera source and hardware admission |
| [xiaomi-uke-firmware](xiaomi-uke-firmware) | File-level firmware provenance and redistribution gates |
| [uke-desktop-integration](uke-desktop-integration) | Measured Uke desktop integration |
| [plasma-uke-kcm](plasma-uke-kcm) | Future native Uke capability panel |
| [uke-hardware-provenance](uke-hardware-provenance) | Public hardware evidence records |

Never clone, fork or rebuild KDE desktop applications as Uke variants. Original
distribution KDE applications remain the only eligible application sources;
their current Python payloads prevent complete graphical admission. A scaffold
does not establish a published RPM or device support.

The kernel, Fedora builder, OrangeFox device tree and Project Aloha remain at
the workspace root because they own independent platform sources and archives.
See the [package hub](../docs/PACKAGE-HUB.md) and
[catalog](../manifests/package-catalog.json) for current build readiness.
