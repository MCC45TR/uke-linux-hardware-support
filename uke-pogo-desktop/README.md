# Uke pogo backlight desktop integration

5 November2026: original native C++ helper for the standard kernel LED, UPower
and original Fedora KDE packages. No KDE/GNOME source or package is forked.

The kernel creates uke-pogo::kbd_backlight only for a reported attached
keyboard. UPower1.91.5 handles LED add/remove. PowerDevil6.8.2 queries keyboard
support only when constructed, so an absent device at login otherwise remains
unsupported after connection. The Plasma applet already handles its service
and supported-action changes.

This unprivileged session helper admits exact Uke/SM7675, KDE, the exact bound
NanoSIC OF node and the100-level LED. It waits for UPower's matching native
object and composite API, then requests TryRestartUnit of the existing
plasma-powerdevil.service only if its cached capability is stale. One attempt
per attachment epoch prevents restart loops. Other backlit keyboards remain
supported after pogo removal. It never activates a stopped PowerDevil, writes
hardware, reads authentication, changes power profiles or accesses a TEE.

UPower LED signals and service owner changes wake a bounded settling timer;
there is no continuous idle polling. KDE autostart starts one static user unit.
--check reports current admitted LED/UPower/PowerDevil state without a restart.
The user unit has no privileged capability. --bridge-attached is a read-only
condition for the existing authentication provider, with no desktop requirement.
A udev rule activates that provider only for the admitted attachment LED; its
device binding stops the provider on detach. Firmware/bridge/TEE admission in
the existing provider remains intact. The bounded restart limit avoids a failed
TA causing a retry loop. This activation needs the new conditional-LED kernel;
the explicit attached-health condition also refuses an old unconditional LED.
The condition returns255 when not ready, using systemd's bounded failure
restart path if LED registration races driver-core dev_groups. Exit1 would
silently skip that retry. No TEE operation starts before exact admission.
Native builds and filesystem/state
fixtures run in COPR; delivery uses DNF. Effective light, live plug/unplug UI
and exact accessory operation require physical qualification.

COPR11262350 qualifies source1ced905bad6dfb9b2ece00ab40e4fc1eaf994928:
native AArch64 Qt6 build, both compiler/sanitizer policy fixtures and full
signed RPM audit pass. qualification-20261105.json separates these results
from pending DNF delivery and the unavailable physical keyboard. The earlier
passed package0.1-1 is superseded by0.1-2, whose TryRestartUnit also preserves
a concurrently stopped PowerDevil service. Version0.1-3 additionally corrects
the attachment-condition readiness race; it needs its own fresh COPR check
before delivery. The0.1-2 receipt remains scoped to its exact source.

Sources: original KDE PowerDevil v6.8.2 keyboardbrightnesscontroller.cpp and
applets/brightness/plugin/keyboardbrightnesscontrol.cpp; UPower v1.91.5
src/up-daemon.c, src/up-device-kbd-backlight.c and linux/up-kbd-backlight-led.c;
Linux LED class keyboard-backlight naming API. Existing desktop packages are
consumed through their public D-Bus/systemd interfaces.
