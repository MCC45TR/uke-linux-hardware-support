#!/bin/bash
# Metadata fixtures only; no upstream helper or target payload is executed.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
for package in uke-displayport-audio uke-speaker-controls uke-gpu-defaults uke-bluetooth uke-speaker-orientation uke-core-meta uke-front-color uke-proximity-capture uke-installation-profile libssc uke-battery-observer uke-touchscreen uke-ssc-fileservice iio-sensor-proxy upower uke-vaapi-v4l2 uke-orangefox-recovery uke-image-tools; do
 bash "$root/packaging/ubuntu/prepare-main-overlay.sh" "$package" "$work/$package"
 diff -ru "$work/$package" "$root/packaging/ubuntu/main-recipes/$package/debian"
 grep -Fxq '3.0 (native)' "$work/$package/source/format"
 grep -q 'bash debian/verify-main-inputs' "$work/$package/rules"
 # Snapshot integration retains every release dependency, including missing
 # hardware ABI providers. It must not bypass normal runtime admission.
 cmp "$work/$package/control" "$root/packaging/ubuntu/recipes/$package/debian/control"
 dpkg --compare-versions "$(dpkg-parsechangelog --file "$work/$package/changelog" -SVersion)" gt "$(dpkg-parsechangelog --file "$root/packaging/ubuntu/recipes/$package/debian/changelog" -SVersion)"
done
cmp "$work/libssc/prepare-libssc" "$root/packaging/ubuntu/prepare-main-libssc.sh"
grep -Fxq 'Source: https://codeberg.org/DylanVanAssche/libssc/archive/v0.4.4.tar.gz' "$work/libssc/copyright"
grep -Fxq 'Files: source/src/ssc-cache.cpp' "$work/libssc/copyright"
printf '%s  %s\n' 716d6bd6b34d2d753060c6b54c9a87e34fae75b724c763bf9ef487efa3621587 "$root/uke-sensors/referances/libssc/libssc-v0.4.4.tar.gz" | sha256sum -c -
echo 'Eighteen canonical main overlays preserve dependencies and upgrade order; native helper execution is separate.'
grep -E '^Build-Depends: .*libssc-dev .*uke-libssc-dual-light-abi-1' "$root/packaging/ubuntu/main-recipes/iio-sensor-proxy/debian/control"
