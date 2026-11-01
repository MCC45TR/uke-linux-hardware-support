#!/bin/bash
# Host source assembly only: known archives, common patches, no network or
# upstream programs. Run from a guarded canonical-main recipe source root.
set -euo pipefail
export LC_ALL=C
package=${1:?source package required}
case $package in
 uke-touchscreen)
  while IFS=$'\t' read -r path hash; do
   printf '%s  %s\n' "$hash" "source/referances/nt36532-thp/$path" | sha256sum -c -
  done < debian/main-touch-inputs.tsv
  bash source/src/prepare-touch-core.sh source/referances/nt36532-thp
  exit 0;;
 uke-ssc-fileservice|iio-sensor-proxy|upower|uke-vaapi-v4l2|uke-image-tools) ;;
 *) echo 'No reviewed main upstream adapter' >&2; exit 78;;
esac
row=$(awk -F '\t' -v p="$package" '$1==p{print}' debian/main-upstreams.tsv)
[[ -n $row && $row != *$'\n'* && ! -e upstream ]]
IFS=$'\t' read -r unused archive hash patchset count url <<< "$row"
[[ $archive == referances/* && $archive != *..* && $hash =~ ^[0-9a-f]{64}$ && $count =~ ^[0-9]+$ ]]
printf '%s  %s\n' "$hash" "source/$archive" | sha256sum -c -
work=$(mktemp -d .main-upstream.XXXXXX)
trap 'rm -rf -- "$work"' EXIT
tar -tf "source/$archive" > "$work/members"
! grep -Eq '(^/|(^|/)\.\.(/|$))' "$work/members"
mkdir "$work/tree"
tar -xf "source/$archive" -C "$work/tree" --strip-components=1
if [[ $count != 0 ]]; then
 [[ $patchset == . || $patchset =~ ^[a-z0-9-]+$ ]]
 shopt -s nullglob
 patches=(source/patches/"$patchset"/*.patch)
 [[ ${#patches[@]} == "$count" ]]
 for patch_file in "${patches[@]}"; do
  patch --batch --fuzz=0 --no-backup-if-mismatch -d "$work/tree" -p1 < "$patch_file"
 done
fi
if [[ $package == iio-sensor-proxy ]]; then
 cp source/src/dual-light.h "$work/tree/src/dual-light.h"
elif [[ $package == uke-image-tools ]]; then
 printf '%s\n' 'Repository: https://github.com/MCC45TR/uke-fedora-builder' \
  'Revision: 9e4d2153e116419c474d0c73eb2280447bf45243' 'Component: uke-image-tools-packaging' > "$work/tree/SOURCE-ORIGIN"
fi
mv "$work/tree" upstream
