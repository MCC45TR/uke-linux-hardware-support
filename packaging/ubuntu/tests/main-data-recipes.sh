#!/bin/bash
# Metadata fixtures only; no upstream helper or target payload is executed.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
for package in uke-displayport-audio uke-speaker-controls uke-gpu-defaults uke-bluetooth uke-speaker-orientation uke-core-meta uke-front-color uke-proximity-capture uke-installation-profile; do
 bash "$root/packaging/ubuntu/prepare-main-overlay.sh" "$package" "$work/$package"
 diff -ru "$work/$package" "$root/packaging/ubuntu/main-recipes/$package/debian"
 grep -Fxq '3.0 (native)' "$work/$package/source/format"
 grep -q 'bash debian/verify-main-inputs' "$work/$package/rules"
 # Snapshot integration retains every release dependency, including missing
 # hardware ABI providers. It must not bypass normal runtime admission.
 cmp "$work/$package/control" "$root/packaging/ubuntu/recipes/$package/debian/control"
 dpkg --compare-versions "$(dpkg-parsechangelog --file "$work/$package/changelog" -SVersion)" gt "$(dpkg-parsechangelog --file "$root/packaging/ubuntu/recipes/$package/debian/changelog" -SVersion)"
done
echo 'Nine canonical main overlays preserve dependencies and upgrade order; native helper execution is separate.'
