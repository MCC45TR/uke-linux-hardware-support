# Uke boot integration

Image input templates and a native read-only boot prerequisite inspector for
Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

`uke-boot-integration` supplies the `uke_esp`/`uke_linux` role contract, EXT4/FAT
fstab template, kernel command line and portable dracut settings. A host image
composer must apply them explicitly. Package installation does not change
fstab, generate an initramfs, select a boot entry, modify EFI variables or write
Android partitions. It includes no UEFI or proprietary firmware binary.

`uke-boot-status` inspects the current EFI presence, exact Uke compatible strings,
selected kernel release and root command line without writing. `--root` supports
an extracted inspection fixture. Exit 2 reports missing prerequisites. Positive
observations do not grant storage, peripheral or physical acceptance.

The independent Uke UEFI port, exact firmware RAM handoff, UFS DT/binding support,
measured partition geometry and Android return remain separate prerequisites.
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

The first Core image targets CDC ACM serial debugging. The package provides an
inactive `uke-cdc-acm.service` and a host-applied override for the distribution's
original `debug-shell.service`. The Core debug composer explicitly enables them
and sets `senemos.debug=cdc-acm`. This development profile exposes a local root
shell over USB once Linux, its UDC and `g_serial` work; normal package installation
does not enable it. Native `--cdc` inspection rejects foreign devices, missing
or ambiguous controllers, and missing ttyGS0 before the corresponding stage.
No Type-C role, security firmware, EFI variable or partition is changed.

Compiled CDC modules do not establish a working Uke USB controller. The current
minimal DTB has no enabled USB controller, so physical CDC acceptance is still
open. See the [upstream serial-gadget protocol](https://docs.kernel.org/usb/gadget_serial.html).
