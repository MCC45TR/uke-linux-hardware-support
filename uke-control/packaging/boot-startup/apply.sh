#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 2 ]]
control=$(realpath "$1");overlay=$(realpath "$2")
[[ ! -e $overlay/legacy && -f $control/src/backend.cpp ]]
cp -a --reflink=auto "$control" "$overlay/legacy"
(cd "$control";patch -p2 --fuzz=0 --forward --dry-run -i "$overlay/consumer.patch";patch -p2 --fuzz=0 --forward -i "$overlay/consumer.patch")
(cd "$control";sha256sum -c --strict --quiet "$overlay/post.sha256")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for file in src/backend.cpp src/backend.hpp src/setup-cdc.cpp tests/setup-cdc.cpp;do
 install -Dm644 "$control/$file" "$work/$file"
done
(cd "$work";patch -p2 --fuzz=0 --reverse -i "$overlay/consumer.patch")
for file in src/backend.cpp src/backend.hpp src/setup-cdc.cpp tests/setup-cdc.cpp;do cmp "$overlay/legacy/$file" "$work/$file";done
grep -Fxq 'void swap_apply(){Lock lock(true);apply_locked(policy());}' "$control/src/backend.cpp"
grep -Fxq 'Result cdc_set(bool enabled, bool wait=false);' "$control/src/backend.hpp"
grep -Fq 'Lock lock(wait);' "$control/src/backend.cpp"
install -Dm644 "$overlay/tests/boot-lock.sh" "$control/tests/boot-lock.sh"
echo 'Control26 changes only boot lock admission and the CDC wait argument; all four complete source preimages are recovered by inversion.'
