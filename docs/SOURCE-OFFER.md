# Exact recovery source and attribution

Initial alpha device source:
https://github.com/MCC45TR/orangefox_device_xiaomi_uke/tree/96e49e0e602de7b3b734bc07e305efca7bcab0e5

The delivered `ORANGEFOX-SOURCE-PINS.xml` lists the exact OrangeFox/AOSP revisions.
The release `ARTIFACT-MANIFEST.json` and `STOCK-KERNEL-SOURCE.json` identify
kernel and utility sources. Corresponding public source snapshots are:

- [RECOVERY-UTILITY-SOURCES.tar.gz](https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/r12.0-uke.20260930-alpha1/RECOVERY-UTILITY-SOURCES.tar.gz), SHA-256 `adb0a1ba413480abb9526f1669f83491eed4c2c1fb43f37c7d1f631063e5cffc`.
- [STOCK-GKI-SOURCE.tar.gz](https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/r12.0-uke.20260930-alpha1/STOCK-GKI-SOURCE.tar.gz), SHA-256 `a0cb69da36f5ca2399c7e7f3c109258594e2fb597bc56290d92723a0ab5500f3`.

OrangeFox/TWRP, Linux, recovery utilities and AOSP files retain their own
copyrights and licenses. Device integration and original delivery documentation
use MIT. The source offer accompanies the data package; upstream source archives
are not installed on the tablet and remain available through the pinned release.

The alpha release documentation records no redistributed vendor HAL blobs,
stock vendor_boot or proprietary firmware. This packaging does not introduce
additional blobs or calibration. Source files are evidence and are not executed
as donor shortcuts. The source RPM reproduces the image delivery package from
the pinned artifacts; a fresh complete OrangeFox source build is a separate job.

For each later package, `release-lock.json` gives the exact current source
snapshot URLs, sizes and SHA-256 values. The links above retain the historical
alpha offer. Standard GPL, Apache, MIT and BSD license texts are included in
`licenses/`, with their SPDX source commit recorded in `licenses/SOURCE.json`.
