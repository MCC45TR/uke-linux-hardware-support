#!/bin/bash
# Derive the remaining reviewed native snapshot metadata. No upstream code runs.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
package=${1:?package required}
output=${2:?new output directory required}
declare -A expected=(
 [uke-battery-observer]=1.4.1-0ubuntu1~tenacious1
 [uke-touchscreen]=0.1.0-0ubuntu1~tenacious1
 [uke-ssc-fileservice]=2026.09.21-0ubuntu1~tenacious2
 [iio-sensor-proxy]=3.9-1ubuntu2~uke1~tenacious1
 [upower]=1.91.5-1ubuntu1~uke1~tenacious1
 [uke-vaapi-v4l2]=1.0.0-0ubuntu1~tenacious1
 [uke-orangefox-recovery]=12.0~preview3.20261010-0ubuntu1~tenacious1
 [uke-image-tools]=1.0.0-0ubuntu1~tenacious1
)
[[ -n ${expected[$package]:-} && ! -e $output ]]
canonical=$root/packaging/ubuntu/recipes/$package/debian
version=$(dpkg-parsechangelog --file "$canonical/changelog" -SVersion)
[[ $version == "${expected[$package]}" ]]
snapshot=${version/-/+uke}
mkdir -p "$output"
cp -a "$canonical/." "$output/"
cp "$root/packaging/ubuntu/recipes/uke-speaker-firmware/debian/verify-main-inputs" "$output/verify-main-inputs"
printf '3.0 (native)\n' > "$output/source/format"
sed "1s/($version)/($snapshot)/" "$canonical/changelog" > "$output/changelog"
case $package in
 iio-sensor-proxy|upower|uke-vaapi-v4l2|uke-ssc-fileservice|uke-image-tools|uke-touchscreen)
  cp "$root/packaging/ubuntu/prepare-main-upstream.sh" "$output/prepare-upstream"
  cp "$root/packaging/ubuntu/main-upstreams.tsv" "$output/main-upstreams.tsv"
  cp "$root/packaging/ubuntu/touch-inputs.tsv" "$output/main-touch-inputs.tsv";;
esac
case $package in
 iio-sensor-proxy|upower|uke-vaapi-v4l2)
  # Replace the old source-root cleanup with cleanup of the prepared tree.
  awk '/^override_dh_auto_clean:/{skip=1;next} skip && /^[^[:space:]#][^:]*:/{skip=0} !skip{print}' "$canonical/rules" |
   sed -e 's|dh $@ --buildsystem=meson|dh $@ --buildsystem=meson --sourcedirectory=upstream|' \
       -e '/cp uke\/src\/dual-light.h src\/dual-light.h/d' \
       -e "/^override_dh_auto_configure:/a\\\t bash debian/verify-main-inputs\n\t bash debian/prepare-upstream $package" \
       -e 's|dh_auto_configure -- |dh_auto_configure --sourcedirectory=upstream -- |' > "$output/rules"
  cat >> "$output/rules" <<'RULES'

override_dh_auto_clean:
	bash debian/verify-main-inputs
	if test -d upstream; then dh_auto_clean --sourcedirectory=upstream; fi
	rm -rf upstream
RULES
  ;;
 uke-ssc-fileservice)
  sed -e "/^override_dh_auto_configure:/a\\\t bash debian/verify-main-inputs\n\t bash debian/prepare-upstream $package" \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|rm -rf build$|rm -rf build upstream|' \
      -e 's|-I include |-I upstream/include |' \
      -e 's|libhexagonrpc/|upstream/libhexagonrpc/|g' \
      -e 's|hexagonrpcd/|upstream/hexagonrpcd/|g' \
      -e 's|uke/src/|source/src/|g' -e 's|uke/packaging/|source/packaging/|g' "$canonical/rules" > "$output/rules"
  sed -i 's|^uke/|source/|' "$output/uke-ssc-fileservice.install";;
 uke-battery-observer)
  sed -e '/^override_dh_auto_configure:/a\	 bash debian/verify-main-inputs' \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|dh_auto_build -- |dh_auto_build --sourcedirectory=source -- |' \
      -e 's|dh_auto_install --destdir=|dh_auto_install --sourcedirectory=source --destdir=|' \
      -e 's|rm -rf build$|rm -rf build source/build|' "$canonical/rules" > "$output/rules";;
 uke-touchscreen)
  sed -e "/^override_dh_auto_configure:/a\\\t bash debian/verify-main-inputs\n\t bash debian/prepare-upstream $package" \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|rm -rf build$|rm -rf build source/src/upstream|' \
      -e 's| src/| source/src/|g' "$canonical/rules" > "$output/rules"
  sed -i 's|^src/|source/src/|' "$output/uke-touchscreen.install";;
 uke-image-tools)
  sed -e "/^override_dh_auto_configure:/a\\\t bash debian/verify-main-inputs\n\t bash debian/prepare-upstream $package" \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|rm -rf build-tools$|rm -rf build-tools upstream|' \
      -e 's| src/| upstream/src/|g' \
      -e 's|install -Dm644 SOURCE-ORIGIN |install -Dm644 upstream/SOURCE-ORIGIN |' "$canonical/rules" > "$output/rules";;
 uke-orangefox-recovery)
  sed -e '/^override_dh_auto_configure:/a\	 bash debian/verify-main-inputs' \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|release|source/referances/releases/R12.0-uke-modded-v003|g' "$canonical/rules" > "$output/rules";;
esac
source_url=$(awk -F '\t' -v p="$package" '$1==p{print $6}' "$root/packaging/ubuntu/main-upstreams.tsv")
[[ -n $source_url ]] || source_url=https://github.com/MCC45TR/uke-linux-hardware-support
sed -e "s|^Source: See.*$|Source: $source_url|" \
    -e 's|^Files: uke/src/|Files: source/src/|' \
    -e 's|^Files: uke/\* debian/\*|Files: source/src/* source/packaging/* source/manifests/* source/tests/* source/docs/* debian/*|' \
    -e 's|^Files: src/upstream/|Files: source/src/upstream/|' \
    -e 's| release/| source/referances/releases/R12.0-uke-modded-v003/|g' "$canonical/copyright" > "$output/copyright"
cat >> "$output/copyright" <<'COPYRIGHT'

Files: AGENTS.md README.md LICENSE debian/*
Copyright: 2026 MCC45TR
License: MIT
COPYRIGHT
if [[ $package == uke-battery-observer ]]; then
 cat >> "$output/copyright" <<'COPYRIGHT'

Files: source/referances/upower/* source/patches/upower/*
Copyright: UPower contributors; see retained upstream notices
License: GPL-2+
 The full GNU General Public License version 2 is available in
 /usr/share/common-licenses/GPL-2 on Debian systems.
COPYRIGHT
fi
bash "$root/packaging/ubuntu/append-main-reference-copyright.sh" "$package" "$output/copyright"
