#!/bin/bash
# Static source-contract checks only; never build or run device code.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
jq -e 'length == 43 and (map(.name)|unique|length)==43 and all(.[]; .status != "ready")' "$root/packaging/ubuntu/coverage.json" >/dev/null
count=0
while IFS=$'\t' read -r package component version; do
 bash "$root/packaging/ubuntu/prepare-source.sh" "$package" "$work/$package"
 dsc=("$work/$package/"*.dsc)
 dpkg-source --no-check -x "${dsc[0]}" "$work/$package-extracted"
 stage=$work/$package-extracted
 debian=$stage/debian
 [[ $(dpkg-parsechangelog -l"$debian/changelog" -S Distribution) == tenacious ]]
 grep -qx 'Architecture: arm64' "$debian/control"
 ! grep -Eq 'dnf|rpm|dracut|selinux|python' "$debian/control"
 find "$debian" -type f -name '*.postinst' -exec sh -n {} \;
 ! grep -ER --include=rules --include='*.postinst' --include='*.preinst' --include='*.postrm' --include='*.prerm' --include=postinst --include=preinst --include=postrm --include=prerm 'efibootmgr|flashrom|fastboot|update-initramfs|systemctl (start|enable)' "$debian"
 case "$package" in
  uke-orangefox-recovery)
   (cd "$stage/release"; sha256sum -c PACKAGE-SHA256SUMS)
   [[ ! -e $debian/postinst && ! -e $debian/preinst && ! -e $debian/postrm && ! -e $debian/prerm ]]
   [[ -s $stage/release/SOURCE-OFFER.md && -d $stage/release/licenses ]]
   ! grep -q '^Provides:' "$debian/control" ;;
  uke-front-color) grep -q 'dh_installsystemd --no-enable --no-start' "$debian/rules" ;;
  iio-sensor-proxy)
   [[ $(wc -l < "$debian/patches/series") == 2 ]]
   grep -q "dual-light.h" "$stage/src/drv-ssc-light.c"
   grep -q "uke-sensorproxy-dual-light-abi-1" "$debian/control" ;;
  uke-vaapi-v4l2)
   [[ $(wc -l < "$debian/patches/series") == 6 ]]
   grep -q -- "-Dfastcv=disabled" "$debian/rules" ;;
  uke-battery-observer)
   grep -q 'integrated_protection_inventory' "$stage/src/battery/control-profile.cpp"
   grep -q 'Source-SHA256: 59ddb88df4139a1ba9e893ac23975ef6885582afcbfa1ed2d4d8b68c42573839' "$stage/RELEASE-ORIGIN"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" ge 1.4.1
   grep -q "dh_installsystemd -puke-battery-observer --no-start" "$debian/rules"
   grep -q "dh_installsystemd -puke-charge-control --no-enable --no-start" "$debian/rules"
   jq -e ".enable_control==false and .qualification==\"unqualified\"" "$stage/manifests/control-profile.json" >/dev/null ;;
  uke-touchscreen)
   [[ $(sha256sum "$stage/src/upstream/nt36532-thp/nvt_touch_core.cpp" | cut -d " " -f1) == 7619b12a440be1f64cacb47b4b617fcfcf4486853822323f8d143c6e73aba9b1 ]]
   grep -q "uke-kernel-touch-thp-abi-1" "$debian/control"
   [[ ! -d $stage/var/lib/uke-touch ]] ;;
  upower)
   [[ $(wc -l < "$debian/patches/series") == 5 ]]
   grep -q "uke-upower-battery-abi-1" "$debian/control"
   grep -q -- "-Dinstalled_tests=false" "$debian/rules"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" gt 1.91.5-1 ;;
  libssc)
   [[ $(wc -l < "$debian/patches/series") == 7 ]]
   grep -q "get_option('tests')" "$stage/meson.build"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" gt 0.4.4-2 ;;
  uke-ssc-fileservice)
   [[ $(sha256sum "$stage/uke/src/ssc-cache.cpp" | cut -d ' ' -f1) == b135389fb30f54ea632db23f6bb1a0a5bd99cb399e19fd74725a791c68bf0afc ]]
   [[ $(sha256sum "$stage/uke/tests/ssc-cache.cpp" | cut -d ' ' -f1) == 1f0a06bfa6d8b95dcf8c801d6922de1e74792d10e5001ace7402237ee0f7feb3 ]]
   [[ $(wc -l < "$debian/patches/series") == 6 ]]
   cmp "$debian/uke-ssc-fileservice.udev" "$stage/uke/packaging/systemd/70-uke-ssc.rules"
   grep -q 'uke-sensorproxy-dual-light-abi-1' "$debian/control"
   grep -q 'uke-ssc-firmware-inputs-abi-1' "$debian/control" ;;
  *) grep -q 'dh_installsystemd --no-start' "$debian/rules" ;;
 esac
 # Verify declared static install sources; generated binaries are checked later.
 for list in "$debian/"*.install; do
  [[ -f $list ]] || continue
  while read -r pattern destination; do
   [[ $pattern == build/* || $pattern == usr/* || $pattern == etc/* ]] && continue
   compgen -G "$stage/$pattern" >/dev/null
  done < "$list"
 done
 (cd "$work/$package"; sha256sum -c SHA256SUMS)
 count=$((count+1))
done < "$root/packaging/ubuntu/components.tsv"
if bash "$root/packaging/ubuntu/prepare-source.sh" unknown "$work/unknown" 2>/dev/null; then exit 1; fi
if bash "$root/packaging/ubuntu/prepare-source.sh" uke-front-color "$work/uke-front-color" 2>/dev/null; then exit 1; fi
# No private per-device calibration package can enter public source preparation.
if bash "$root/packaging/ubuntu/prepare-source.sh" uke-local-touch-data "$work/private" 2>/dev/null; then exit 1; fi
printf 'PASS: %s source packages, extraction, patch application, metadata, lifecycle, checksums and rejection cases\n' "$count"
