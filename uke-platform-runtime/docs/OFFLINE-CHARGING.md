# Offline charging

18 October 2026; source implementation, physical qualification pending.

The separate `uke-offline-charge` package draws a native battery interface on the
internal DRM output. The matched Uke initramfs selects `uke-offline-charge.target`
only for an explicit `androidboot.mode=charger` signal and the exact Uke/SM7675
identity. Kernel, minimal system services and the firmware needed for charging
remain active; KDE, networking and the normal multi-user target are not selected.
This is a minimal Linux charging mode, not an interface that runs without a CPU.

The screen reports the actual battery percentage and status. Unknown data remains
unknown; a reported 100% is not converted into Full or charge-stop acceptance.
The UI sends no charging, current, voltage, vendor or PD commands. The firmware
retains charging and autonomous protection ownership. Independent 80% policy,
actual cutoff, bypass, temperature qualification and power use remain separate.

Battery power is the standard signed battery-side `power_now` in microwatts,
not adapter input or negotiated maximum power. Temperature uses the standard
deci-Celsius value. Time to full uses the firmware's positive `time_to_full_avg`
only while actually Charging with input present. It is not a qualified prediction
to an 80% target. Unavailable inputs show dashes; no percentage-only time or
adapter-wattage estimate is invented.

Hold the power key for two seconds to request a normal systemd reboot.
Confirmed USB input loss across two samples requests normal systemd poweroff.
Missing input data never becomes an unplug event. The display attempts standard
DPMS blanking after ten seconds and a key press wakes it. A busy DRM master is
refused; the application must not run over an active desktop. The service has no
enable preset and does not restart itself on failure. Boot-mode status is a
read-only `uke-offline-charge --status` query.

The initial implementation requires root firmware and modules, so it mounts the
existing Fedora root before selecting the charging target. Charging with a
missing or damaged root, exact bootloader signal delivery, cold unplug/power-key
behavior, low-battery startup and long-duration efficiency require independent
device tests. No bootloader, devinfo, charger partition or calibration is written.

Source comparisons: Aloha Platforms `8d37e2bfab2f9d959ac0b3d68434e8be4866bf55`,
`QcomModulePkg/Library/BootLib/UpdateCmdLine.c`, supplies an explicit charger mode
for `PauseAtBootUp`; this is boot-infrastructure evidence, not exact stock ABL
qualification. Uke OEM `5712080a4b0c0b1a879e6d145c601c649ddaddfd` guards
`charger_partition_poweroffmode` behind other product options; that persistent
writer is not used. Original systemd's `switch-root` and target interfaces retain
normal service/shutdown ordering. No donor program or UI code is executed/copied.
