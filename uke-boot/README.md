# Uke boot integration

Image input templates and a native read-only boot prerequisite inspector for
Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

`uke-boot-integration` supplies the fastboot `esp`/`linux` role contract, EXT4/FAT
fstab template, kernel command line and portable dracut settings. A host image
composer must apply them explicitly. Package installation does not change
fstab, generate an initramfs, select a boot entry, modify EFI variables or write
Android partitions. It includes no UEFI or proprietary firmware binary.

`uke-boot-status` inspects the current EFI presence, exact Uke compatible strings,
selected kernel release and root command line without writing. `--root` supports
an extracted inspection fixture. Exit 2 reports missing prerequisites. Positive
observations do not grant storage, peripheral or physical acceptance.

Images use filesystem labels `UKE_ESP` and `UKE_LINUX`, with composer-selected
sizes and no fixed GPT offsets. Target fastboot capacity, sector size and UEFI
partition visibility still require inspection before installation. The native
inspector accepts `root=PARTLABEL=linux`, `root=LABEL=UKE_LINUX` and the legacy `PARTLABEL=uke_linux`
selector; a later foreign root argument overrides either one and is rejected.

The independent Uke UEFI port, exact firmware RAM handoff, UFS DT/binding support
and Android return remain separate prerequisites.
Nabu binary/geometry defaults cannot supply them. Detailed records are in
[private engineering documentation](https://github.com/MCC45TR/uke-linux-docs).

Run `make validate`, `make test-native` or `make srpm`. Source archives remain in
`referances/`; package/host output is ignored under `build/`.

Release 7 disables readline editing and systemd terminal reset/hangup for the
development ACM shell. A production-equivalent runtime override stopped in
42 ms in two physical trials while retaining the bounded identity/TTY gate.
Startup still takes about 15 seconds; bidirectional ACM data remains unqualified.

Release 6 removes the system-wide Python conflicts from both native packages.
A development exception covers Python in signed, original Fedora KDE
dependencies. A compliant Python-free KDE release remains blocked. Neither package contains or requires Python, and project startup,
service and test helpers remain native or shell. The default Core image and
early initramfs retain their independent no-Python audits. A KDE composer must
record the original signed package closure and audit its extracted payload;
this packaging change alone does not authorize arbitrary Python applications
or establish a working graphical session.

The host-only `src/audit-kernel-inputs.sh EXTRACTED_ROOT REPORT_JSON` checks the
installed kernel configuration and exact Uke DTB, traverses enabled nodes and
records static RAM, UFS and firmware requests. It requires Bash, jq and the
device-tree-compiler utilities. Exit 2 records incomplete static inputs; a
future verified UEFI RAM fixup needs independent evidence. This host audit is
excluded from the binary package. Synthetic fixtures also reject firmware
requests below disabled parents and foreign device trees.

The earlier ESP32 Core development profile uses the **tablet as USB host**, with an
ESP32-S3 bridge exposing CDC and HID interfaces. `uke-esp32-cdc` supplies a native
raw 115200 TTY forwarder and inactive `uke-esp32-cdc-log.service`. The explicitly
configured port defaults to `/dev/ttyACM0`; change `UKE_ESP32_CDC_DEVICE` in
`/etc/senemos/esp32-cdc.conf` if enumeration differs. An interface pathname is
configuration, not verified ESP32 identity.

The compositor applies `debug-shell-esp32.conf` to the distribution's original
`debug-shell.service`, enables the real-root log forwarder and sets
`senemos.debug=esp32-cdc`. HID selects VT2 and enters shell commands; stdout and
stderr go through journald to the single CDC writer. This is an explicit local
root development shell. The selected bridge firmware and physical HID/CDC
behavior still need separate validation. The image builder can additionally
include its gated `uke-bringup` dracut module for VT2 and the single CDC writer
before mounting the Linux root filesystem. The real-root service excludes
initramfs execution; installation alone enables neither stage. No Python
utility is used.

The optional `uke-cdc-acm.service` and `debug-shell-cdc.conf` describe the
opposite tablet-gadget mode (`ttyGS0` / `senemos.debug=cdc-acm`). They remain
inactive in the ESP32-S3 host profile. Native `--cdc` checks belong to that
optional mode; `--identity` checks the Uke/kernel prerequisite for the host
profile. Neither package installation nor native inspection changes Type-C
roles, security firmware, EFI variables or partitions.

The current minimal DTB has no enabled USB controller. Compiled host/HID/ACM or
gadget modules therefore do not establish physical bridge connectivity. The
USB platform/PHY/clock and firmware handoff remain source prerequisites.

The stock-ABL development Image is a separate boot profile. On 2026-10-11 a local TTY was visible on the Fedora root. Direct-PC CDC/ECM
commands and SSH
completed on the same physical boot. This does not qualify the standalone
mainline DTB, USB host power, GPU acceleration or Wi-Fi.

Release 5 contains an active `modprobe.d` containment for unqualified OEM
CoreSight autoload. The same Image previously reset after CoreSight probes;
the scoped blacklist allowed the first sustained terminal. The precise failing
CoreSight register access and repeated cold-boot acceptance remain open.

`uke-usb-network.service` and `uke-usb-network-shell.service` are inactive until
the image composer installs `usb-network.nmconnection` as
`/etc/NetworkManager/system-connections/uke-usb-debug.nmconnection` (root, 0600)
and `usb-network-managed.conf` as
`/etc/NetworkManager/conf.d/90-uke-usb-debug.conf` (root, 0644), then reloads
NetworkManager and explicitly enables them with `senemos.debug=cdc-acm`.
The device override is required after re-enumeration because Fedora's udev
policy otherwise leaves the newly created gadget interface unmanaged. They replace the
serial-only gadget with ACM plus ECM, retain `ttyGS0`, and use an isolated
`10.77.0.2/30` link without a default route. Configure the computer as
`10.77.0.1/30`. Authorize an SSH key through the existing terminal and verify the
server host key before connecting. Package installation enables no service and
contains no passwords, authorized keys or private host keys.

The `machine-info` and `fastfetch.jsonc` profiles provide the configured
Xiaomi Pad 7 presentation. Revision 1.1 is configured presentation metadata,
not a measured board-revision result. The Adreno 732 hardware label explicitly
retains the missing DRM qualification. Disk size always comes from the mounted
filesystem; grow an existing EXT4 root only after verifying its physical
`linux` partition. The systemd `reboot bootloader` command already accepts the
argument, but a matching kernel reboot-reason backend and physical fastboot
transition must be qualified before claiming it works.
