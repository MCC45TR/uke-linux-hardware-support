# Uke Tablet Controls

24 October 2026: Plasma 6 applet using the original Fedora desktop and
`uke-control` command interface. No hardware helper, daemon, sysfs writer,
firmware access or Python runtime is included in this package.

The layout and `ControlRow.qml` derive from SENEMOS Nabu Tablet Control,
GPL-3.0-or-later, at [commit 71602f1](https://github.com/MCC45TR/nabu-linux-hardware-support/tree/71602f1a9f559f25e71a0979d0d5058579e2697e/desktop/nabu-tablet-controls/plasma).
Nabu helpers, board settings and hardware claims are excluded. The applet
passes only fixed commands to `uke-control`. Session controls run as the
desktop user. USB requests use the original Polkit administrator prompt and
the backend's existing admission checks; cancellation remains an error.

Automatic rotation and brightness require KScreen's advertised internal DSI control.
Cover state comes from the existing UPower interface. Unsupported flashlight,
missing cover capability and unavailable display controls remain disabled.
USB role changes and persistent serial settings require explicit confirmation.
Status is refreshed when opened or requested, without a background timer.

The separate `uke-plasma-controls` RPM is recommended only by
`uke-kde-plasma-meta`. Removing it retains the KDE meta package. No GNOME
dependency or session integration is installed. Add it through Plasma's
System Tray settings if an existing tray does not enable new entries.
Source, package and physical tablet results are recorded separately.

The package check loads the actual QML component with the original Plasma
imports, without creating the widget or executing device commands. Release 13
sizes the confirmation dialog directly; the label's implicit width is read-only
on the tested original Fedora Qt 6.11 runtime.

30 October 2026 source candidate, release 25: light controls use a small native
Qt D-Bus client on a dedicated persistent connection. It exposes only fixed
status, torch percentage/enable and RGB-test methods; it cannot access a device,
sysfs path or arbitrary bus destination. The original system authorization window
opens for the initial light change. Control binds a fixed two-minute grant to
that exact connection. Closing the popup closes the connection; another process
or widget instance has a different peer identity. Brightness is applied once
when the settings dialog is accepted. An unknown kernel output state is shown
as unavailable, rather than as an assumed zero brightness.

The QML module is part of the removable Uke applet package and uses original
Fedora Qt/Plasma. No KDE application is forked. Native private-bus and QML-load
checks are separate from a live authentication-window or physical light trial.
