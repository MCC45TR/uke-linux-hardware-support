#!/usr/bin/bash
# Source-only overlay for the actual consolidated Control23 boot consumer.
set -Eeuo pipefail
[[ $# == 2 ]]
boot=$(realpath "$1");overlay=$(realpath "$2")
[[ -d $boot/src && -d $boot/tests &&
   ! -e $boot/src/super-profile.hpp && ! -L $boot/src/super-profile.hpp &&
   ! -e $boot/tests/super-boot-update.cpp && ! -e $boot/tests/super-boot-update.sh ]]
(cd "$boot";sha256sum -c --strict --quiet "$overlay/pre.sha256")
before=$(mktemp -d)
trap 'find "$before" -depth -delete' EXIT
for name in kernel-update.cpp userdata-update.hpp select-existing-slot.hpp;do
 (cd "$boot";cp --parents "src/$name" "$before/")
done
(cd "$boot";patch -p1 --fuzz=0 --forward --dry-run < "$overlay/consumer.patch";
 patch -p1 --fuzz=0 --forward < "$overlay/consumer.patch")
install -m644 "$overlay/src/boot-update/super-profile.hpp" "$boot/src/super-profile.hpp"
install -m644 "$overlay/tests/super-boot-update.cpp" "$boot/tests/"
install -m755 "$overlay/tests/super-boot-update.sh" "$boot/tests/"
(cd "$boot";sha256sum -c --strict --quiet "$overlay/post.sha256")
mkdir "$before/inverse"
for name in kernel-update.cpp userdata-update.hpp select-existing-slot.hpp;do
 install -Dm644 "$boot/src/$name" "$before/inverse/src/$name"
done
(cd "$before/inverse";patch -p1 --fuzz=0 --reverse < "$overlay/consumer.patch")
for name in kernel-update.cpp userdata-update.hpp select-existing-slot.hpp;do
 cmp "$before/src/$name" "$before/inverse/src/$name"
done
echo 'Complete Control23 consumer inverse preserved; super profiles and slot annotations require the exact kernel capability.'
