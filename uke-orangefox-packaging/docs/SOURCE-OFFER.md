# Recovery source and attribution

Delivery checkpoint: 14 October 2026. Image bytes are from the published
[v003 source tag](https://github.com/MCC45TR/orangefox_device_xiaomi_uke/tree/02d1431dffc2921797017d9ef7304d4eaf6685a7).
`ORANGEFOX-SOURCE-PINS.xml` retains the OrangeFox/AOSP revisions. The image
build and extracted-payload receipts are identified by SHA-256 in the artifact
manifest; they do not establish binary reproducibility or physical acceptance.

Corresponding source snapshots from the same archived v003 output:

- [RECOVERY-UTILITY-SOURCES.tar.gz](https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/R12.0-uke-modded-v003/RECOVERY-UTILITY-SOURCES.tar.gz), SHA-256 `89ee7f588fdcc739edcf53f0362038bd08569a732cc83f813bc3129bc9eb8862`.
- [STOCK-GKI-SOURCE.tar.gz](https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/R12.0-uke-modded-v003/STOCK-GKI-SOURCE.tar.gz), SHA-256 `49917981e2f300aaa33b02855e3e87673b67280cc2105860d8bba08f4f6728fd`.

The IMG uses the installed Android boot stack; the stock GKI source record
identifies the archived build input, not a kernel embedded in the recovery IMG.
Recovery, utility, Linux and AOSP sources retain their original notices. MIT
covers original delivery documentation. Standard GPL, Apache, MIT and BSD
texts and their SPDX source identity are included in `licenses/`.

Source archives are host inputs and are not installed on the tablet. The SRPM
rebuilds the delivery package from pinned release artifacts; it does not compile
OrangeFox. `release-lock.json` records each asset URL, size and SHA-256. The
withdrawn Alpha 1 artifacts are historical evidence and are not redistributed
by this updated package.
