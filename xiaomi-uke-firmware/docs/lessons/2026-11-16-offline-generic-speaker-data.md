# UKE-AUDIO-DATA-20261116: deliver generic speaker data before playback

16 November 2026. Fresh Uke Fedora on signed kernel28 has the native protected
speaker path and original Fedora PipeWire/WirePlumber. Runtime preparation
fails when its generic Forte ACDB input is absent. Exact OEM public design data
from commit87f59fc5, SHA-256bd3f9b88,2838166bytes allows the retained gain223
path to prepare, and a user confirmed audible playback. This is distinct from
private fitted-unit resistance data, which stays read-only and device-local.

The existing explicit download helper uses an obsolete repository location and
its cache is empty in a fresh image. A manual diagnostic download establishes
the cause, but cannot provide offline first-boot behavior. Keep data delivery in
the existing `uke-firmware` package. Firmware16 derives from actual15/COPR11309337,
not the stale local release2 recipe. Preserve both source archives,217/218-file
baseline checks, all old paths and OEM notices with a complete recipe inverse.
Add only the pinned ACDB file, provenance and a standard tmpfiles configuration.
The existing LicenseRef-Proprietary classification and incomplete file-level
redistribution review remain explicit; project MIT does not relicense the data.

The immutable factory file is under the normal firmware path. Original systemd
tmpfiles copies it to the existing native helper's compatibility cache only when
absent. This requires no new service, polling, network or media-provider fork.
It preserves existing files and links; the native helper still rejects invalid
or unsafe cache entries. At most one extra2838166-byte compatibility copy is
needed per root. No private calibration is placed in the package or cache.

PC data tests use the original systemd executable and synthetic files. Fresh
copy, repeated inode/content preservation, existing content/link preservation
and recreation pass. `--root` prefixes managed paths, but an explicit config
filename must point to the host test file. Also, tmpfiles C skips a missing
source rather than failing: add an explicit regular-file/hash guard in the RPM
post script. The fixture includes that guard and verifies missing-source refusal.
No C/C++ compiler runs on the PC or tablet. Native COPR data/package checks,
signed DNF delivery and cache-free offline reboot remain required.

References: [pinned public OEM directory](https://github.com/Xiaomi-Pad-7-Pro-Resources/android_vendor_xiaomi_uke/tree/87f59fc5c2ef1ee04e494e5258853d36a3346059/proprietary/odm/etc/acdbdata/Forte)
and [original systemd tmpfiles format](https://www.freedesktop.org/software/systemd/man/latest/tmpfiles.d.html).
The retained helper can later consume the immutable path directly; remove the
compatibility copy only after upgrade and original-helper rollback are qualified.
