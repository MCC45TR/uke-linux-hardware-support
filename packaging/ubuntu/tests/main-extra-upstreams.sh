#!/bin/bash
# Source assembly/refusal fixtures only; no compiler or target program runs.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
for package in uke-ssc-fileservice iio-sensor-proxy upower uke-vaapi-v4l2 uke-image-tools; do
 component=$(awk -F '\t' -v p="$package" '$1==p{print $2}' "$root/packaging/ubuntu/components.tsv")
 row=$(awk -F '\t' -v p="$package" '$1==p{print}' "$root/packaging/ubuntu/main-upstreams.tsv")
 IFS=$'\t' read -r unused archive hash patchset count url <<< "$row"
 for scenario in valid corrupt missing-patch broken-patch; do
  [[ $count != 0 || $scenario != *patch ]] || continue
  stage=$work/$package/$scenario
  mkdir -p "$stage/debian" "$stage/source/$(dirname "$archive")"
  cp "$root/packaging/ubuntu/main-upstreams.tsv" "$stage/debian/"
  cp "$root/$component/$archive" "$stage/source/$archive"
  if [[ $count != 0 ]]; then
   cp -a "$root/$component/patches" "$stage/source/"
   patches=("$stage/source/patches/$patchset/"*.patch)
  fi
  if [[ $package == iio-sensor-proxy ]]; then
   mkdir "$stage/source/src"
   cp "$root/$component/src/dual-light.h" "$stage/source/src/"
  fi
  case $scenario in
   corrupt) printf corruption >> "$stage/source/$archive";;
   missing-patch) rm "${patches[0]}";;
   broken-patch) printf 'invalid patch fixture\n' > "${patches[0]}";;
  esac
  if (cd "$stage"; bash "$root/packaging/ubuntu/prepare-main-upstream.sh" "$package") > "$stage/log" 2>&1; then
   [[ $scenario == valid && -d $stage/upstream ]]
   [[ -z $(find "$stage/upstream" -name '*.orig' -o -name '*.rej') ]]
  else
   [[ $scenario != valid && ! -e $stage/upstream ]]
  fi
  [[ -z $(find "$stage" -maxdepth 1 -name '.main-upstream.*') ]]
 done
done
for scenario in valid bad-license; do
 stage=$work/touch/$scenario
 mkdir -p "$stage/source/referances" "$stage/source/src" "$stage/debian"
 cp "$root/packaging/ubuntu/touch-inputs.tsv" "$stage/debian/main-touch-inputs.tsv"
 cp -a "$root/uke-sensors/referances/nt36532-thp" "$stage/source/referances/"
 cp -a "$root/uke-sensors/patches" "$stage/source/"
 cp "$root/uke-sensors/src/prepare-touch-core.sh" "$stage/source/src/"
 [[ $scenario != bad-license ]] || printf corruption >> "$stage/source/referances/nt36532-thp/LICENSE"
 if (cd "$stage"; bash "$root/packaging/ubuntu/prepare-main-upstream.sh" uke-touchscreen) > "$stage/log" 2>&1; then
  [[ $scenario == valid ]]
  printf '%s  %s\n' 7619b12a440be1f64cacb47b4b617fcfcf4486853822323f8d143c6e73aba9b1 "$stage/source/src/upstream/nt36532-thp/nvt_touch_core.cpp" | sha256sum -c -
 else
  [[ $scenario == bad-license && ! -e $stage/source/src/upstream ]]
 fi
done
(cd "$root/uke-platform-runtime"; sha256sum -c manifests/released-source-1.4.1.sha256) > "$work/battery.log"
(cd "$root/uke-orangefox-packaging/referances/releases/R12.0-uke-modded-v003"; sha256sum -c PACKAGE-SHA256SUMS) > "$work/recovery.log"
echo 'Remaining main source/patch/license admission fixtures and exact released payload hashes passed.'
