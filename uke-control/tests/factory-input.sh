#!/usr/bin/bash
# COPR only: same protected loader compiled in both complete production readers.
set -Eeuo pipefail
[[ $# == 3 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
boot=$(realpath "$1");color=$(realpath "$2");overlay=$(realpath "$3")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for reader in color video;do
 if [[ $reader == color ]];then
  body=$color/o82-color-profile.cpp
  source=/usr/lib/firmware/qcom/uke/display/o82-42.json
  defs=(-DUKE_FACTORY_COLOR "-Wl,--wrap=main")
  libs=(json-c lcms2 openssl)
 else
  body=$boot/src/kernel-update.cpp
  source=/usr/lib/firmware/qcom/uke/video/vpu30_2v.mbn
  defs=()
  libs=(fdisk zlib openssl libsystemd)
 fi
 read -r -a link_flags <<< "$(pkg-config --cflags --libs "${libs[@]}")"
 for compiler in g++ clang++;do
  for mode in plain sanitized;do
   flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function)
   [[ $mode != sanitized ]] || flags+=("-fsanitize=address,undefined" -fno-sanitize-recover=all -fno-omit-frame-pointer)
   "$compiler" "${flags[@]}" "${defs[@]}" "-DUKE_FACTORY_BODY=\"$body\"" \
    "$overlay/tests/factory-input.cpp" "${link_flags[@]}" -o "$work/check"
   readelf -h "$work/check" | grep -F AArch64
   timeout --kill-after=2s 45s "$work/check" "$source"
  done
 done
done
