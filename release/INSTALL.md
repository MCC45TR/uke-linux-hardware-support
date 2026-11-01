# OrangeFox v003 preview

Delivery record: 14 October 2026. Target: Xiaomi Pad 7 / POCO Pad X1 (`uke`).
The RPM contains the existing v003 release bytes; no recovery was rebuilt for
this package. The image and ZIP are pinned by SHA-256 in `release-lock.json`.

The recovery IMG contains a ramdisk and uses the installed Android boot stack.
It is not a complete `fastboot boot` image. Firmware and module compatibility
must be checked separately. Do not write this IMG to Linux boot partitions.

ZIP installation is unavailable. The recovery's managed storage-write paths
remain restricted; a successful package build does not accept those operations.
Package installation and upgrades only replace files under `/usr/share`.

Read the current recovery installation and rollback instructions before any
physical trial: https://github.com/MCC45TR/orangefox_device_xiaomi_uke
Preserve a tested recovery and the installed Android early firmware.

Build/payload records accompany the preview. They do not establish cold-boot,
touch, charging, encryption, storage-write or hardware rollback acceptance.
