#!/usr/bin/bash
# COPR/source-only application to the exact prepared Control26 readers.
set -Eeuo pipefail
[[ $# == 3 && ! -e /etc/senemos/kernel-update.conf ]]
boot=$(realpath "$1");color=$(realpath "$2");overlay=$(realpath "$3")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$work/current/boot/src" "$work/current/color" "$work/inverse/boot/src" "$work/inverse/color"
[[ ! -e $boot/src/factory-input.hpp && ! -e $color/factory-input.hpp ]]
cp "$boot/src/oem-video.hpp" "$work/current/boot/src/"
cp "$color/o82-stock-profile.hpp" "$work/current/color/"
(cd "$work/current";sha256sum -c --strict --quiet "$overlay/pre.sha256")
patch --batch --fuzz=0 --dry-run -p1 -d "$work/current" < "$overlay/consumer.patch"
patch --batch --fuzz=0 -p1 -d "$work/current" < "$overlay/consumer.patch"
install -m644 "$overlay/factory-input.hpp" "$work/current/boot/src/"
install -m644 "$overlay/factory-input.hpp" "$work/current/color/"
(cd "$work/current";sha256sum -c --strict --quiet "$overlay/post.sha256")
cp "$work/current/boot/src/oem-video.hpp" "$work/inverse/boot/src/"
cp "$work/current/color/o82-stock-profile.hpp" "$work/inverse/color/"
patch --batch --fuzz=0 -R -p1 -d "$work/inverse" < "$overlay/consumer.patch"
cmp "$work/inverse/boot/src/oem-video.hpp" "$boot/src/oem-video.hpp"
cmp "$work/inverse/color/o82-stock-profile.hpp" "$color/o82-stock-profile.hpp"
install -m644 "$work/current/boot/src/oem-video.hpp" "$boot/src/"
install -m644 "$work/current/boot/src/factory-input.hpp" "$boot/src/"
install -m644 "$work/current/color/o82-stock-profile.hpp" "$color/"
install -m644 "$work/current/color/factory-input.hpp" "$color/"
printf 'Exact prepared reader preimages, whole-file inverse and protected factory loader composition passed.\n'
