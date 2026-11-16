#!/usr/bin/bash
# COPR only; retain the existing userdata fixture's unused-function allowance.
set -Eeuo pipefail
[[ $# == 2 && $(uname -m) == aarch64 ]]
boot=$(realpath "$1");overlay=$(realpath "$2")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for compiler in g++ clang++;do
 for mode in plain sanitized;do
  flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function)
  [[ $mode != sanitized ]] || flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  "$compiler" "${flags[@]}" "$boot/tests/super-boot-update.cpp" \
    $(pkg-config --cflags --libs fdisk zlib openssl libsystemd) -o "$work/check"
  readelf -h "$work/check" | grep -F AArch64
  (cd "$boot";timeout 90s "$work/check")
 done
done
# Compile the complete production CLI without any warning suppression; privilege
# checks precede all file/device work, including the file-only derivation command.
g++ -std=c++20 -O2 -Wall -Wextra -Werror "$boot/src/kernel-update.cpp" \
 $(pkg-config --cflags --libs fdisk zlib openssl libsystemd) -o "$work/uke-kernel-update"
[[ $(id -u) != 0 ]]
if "$work/uke-kernel-update" derive-slot-image /missing /missing a > "$work/refusal" 2>&1;then exit 1;fi
grep -Fxq 'uke-kernel-update: Root privileges are required' "$work/refusal"
(cd "$boot";sha256sum -c --strict --quiet "$overlay/post.sha256")
