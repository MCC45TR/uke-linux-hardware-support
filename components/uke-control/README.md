# Uke Control

Native device status, diagnostics and settings for Xiaomi Pad 7 and
POCO Pad X1 on Fedora. The terminal interface and optional GTK and Qt
applications use the shared C++ client and backend.

- [Terminal interface](docs/guides/terminal-interface.md)
- [Command line](docs/guides/command-line.md)
- [Charging](docs/guides/charging.md), [display and docks](docs/guides/display-and-docks.md), [sensors](docs/guides/sensors.md)
- [Update channels](docs/guides/update-channels.md) and [boot layout](docs/guides/boot-layout.md)
- [Diagnostics](docs/guides/diagnostics.md) and [hardware status](docs/guides/hardware-status.md)

Source lives in `src/`, the interface shared with consumers in `shared/`,
and the RPM definitions in `packaging/`. From the repository root, use
`make -C components/uke-control validate` for source checks. Qualified releases
are distributed as signed Fedora packages.
