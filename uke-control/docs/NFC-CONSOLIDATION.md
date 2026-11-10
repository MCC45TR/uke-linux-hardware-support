# Native NFC and infrared integration in Control

10 November 2026. Control2.16 folds the existing NFC1.1.0-2 client and exact
NFC/IR udev rules into the existing main RPM. The paths /usr/bin/uke-nfc-tag
and /usr/lib/udev/rules.d/70-uke-nfc-ir.rules, client operations, read-only kernel
ABI and active-session ACL behavior remain. Existing kernel providers own the
hardware transport and lifecycle. No new service or hardware bridge is added.

Control provides the old package and both versioned capabilities, and obsoletes
the retired RPM through1.1.0-2. On ARM64 it retains the matching native kernel
capability requirement; the original distribution ir-ctl remains required.
The main package and mandatory TUI share the exact Control version/release.
The same NFC client/UAPI, bounded parsing/errno/poll tests and real temporary
filesystem ACL regression tests run in COPR; device compilation is not used.

The fold changes ownership, not NFC roles. The device remains a Type2 tag,
not an NCI reader or card-emulation provider. Its memory requests are explicitly
bounded and read-only. IR remains a standard LIRC transmitter through ir-ctl;
no receive/learning capability is invented. Installation reloads only the
same rules and requests the same exact misc/LIRC change events; no tag memory
read/write or IR transmission occurs.

Before retiring the separate COPR producer, admit signed RPMs and the normal
DNF replacement, verify sole ownership/capabilities and preserved settings,
compare read-only tag info and IR feature queries with the installed baseline,
and check supported reinstall/fresh/downgrade/recovery paths. RF/optical physical
acceptance is separate and is not inferred from capability queries or packaging.
