# Uke OEM resources

31 October 2026. Optional read-only resource access for POCO Pad X1 on Fedora
Rawhide. Source and package checks are separate from tablet acceptance.

The service mounts modem, DSP and Bluetooth A/B banks separately through
read-only loop devices. It verifies AOSP logical-partition geometry, primary
and backup metadata, SHA-256 checksums, source capacity and extent bounds before
opening the single-linear, read-only LP0 vendor_a and odm_a filesystems. Updated,
disabled, multi-extent and snapshot partitions are not selected. Ext4 mounts
use ro,noload; all mounts use nosuid,nodev,noexec. Sources remain private under
/run/uke-oem-resources. Missing or incompatible resources leave the normal
packaged firmware available. This service is not required by a boot target.

DTBO references are extracted and header/bounds checked, never applied to a
running or saved device tree. LP0 is an explicitly named reference view, not a
claim about the active Android snapshot or the Linux boot bank. WLAN files are
inside modem, not a separate WLAN partition. No Android executable is run.

The selected RAM profile contains only the installed SSC defaults and DSP
helper when they match OEM content byte-for-byte. The optional native SSC cache
consumer rechecks the entire profile against the current package on every use;
a changed package, missing reference or mismatched file selects the package
fallback. Persist remains on its existing read-only calibration path. No kernel
firmware search path, RPM-owned file, regulator, radio or panel setting changes.
The package requires the native OEM-defaults capability so a future update
cannot replace the consumer with a producer that lacks this integration.

Compilation and RPM checks run in COPR. The package contains only source-owned
helpers, units and documentation; it never redistributes device captures.
After installation, enable uke-oem-resources.service explicitly. Its status
helper reports reference compatibility. Stop it before manual source maintenance;
cleanup uses ordinary unmounts and detaches only recorded, verified loop devices.
A busy source is reported and retained, without forced or lazy unmounting.

Format references: AOSP liblp metadata_format.h, blob
8d77097ed67b047603d4571e514730ca98b63cf8 (Apache-2.0), and libufdt dt_table.h,
blob bf149ddb8cb7ab31d1c2211c638693d5349afeb6 (Apache-2.0). No donor program runs.
