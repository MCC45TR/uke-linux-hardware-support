# Uke Linux hardware support

One repository owns twelve Uke/SM7675 package components. Each component is an
ordinary directory with its own sources, rules, license and acceptance gates.
Their original project histories are preserved as Git merge ancestry;
[SOURCE-IMPORTS.json](SOURCE-IMPORTS.json) records all twelve exact import pins.
The workspace checks out this repository at `uke-linux-hardware-support/`.

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

The kernel, Fedora builder, OrangeFox device tree and Project Aloha keep their
independent repositories. See the [package hub](https://github.com/MCC45TR/uke-linux/blob/main/docs/PACKAGE-HUB.md)
for build readiness. No physical support is inferred from this consolidation.

Run `make validate` to check all component contracts and `make test-native` for
the existing C++ configuration migration fixtures. The root COPR rule dispatches
only the ten already admitted support source families. The stable-source
workflow checks Material Decoration and the eligible recovery release daily;
the independent kernel repository retains its own reviewed stable automation.
Original repository workflows remain under each component as historical source
records; active workflows are in this repository's root `.github/workflows/`.
