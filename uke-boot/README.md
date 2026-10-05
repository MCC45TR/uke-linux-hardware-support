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
inspector accepts `root=LABEL=UKE_LINUX` and the legacy `PARTLABEL=uke_linux`
selector; a later foreign root argument overrides either one and is rejected.

The independent Uke UEFI port, exact firmware RAM handoff, UFS DT/binding support
and Android return remain separate prerequisites.
Nabu binary/geometry defaults cannot supply them. Detailed records are in
[private engineering documentation](https://github.com/MCC45TR/uke-linux-docs).

Run `make validate`, `make test-native` or `make srpm`. Source archives remain in
`referances/`; package/host output is ignored under `build/`.

The host-only `src/audit-kernel-inputs.sh EXTRACTED_ROOT REPORT_JSON` checks the
installed kernel configuration and exact Uke DTB, traverses enabled nodes and
records static RAM, UFS and firmware requests. It requires Bash, jq and the
device-tree-compiler utilities. Exit 2 records incomplete static inputs; a
future verified UEFI RAM fixup needs independent evidence. This host audit is
excluded from the binary package. Synthetic fixtures also reject firmware
requests below disabled parents and foreign device trees.

The first Core development profile uses the **tablet as USB host**, with an
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
