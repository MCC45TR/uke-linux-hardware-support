#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 2 ]]
control=$(realpath "$1");overlay=$(realpath "$2")
[[ ! -e $overlay/legacy ]]
(cd "$control";sha256sum -c --strict --quiet "$overlay/pre.sha256")
mkdir "$overlay/legacy"
for file in src/{backend.cpp,backend.hpp,main.cpp};do
 install -Dm644 "$control/$file" "$overlay/legacy/$file"
done
(cd "$control";patch -p2 --fuzz=0 --forward --dry-run -i "$overlay/consumer.patch";patch -p2 --fuzz=0 --forward -i "$overlay/consumer.patch")
(cd "$control";sha256sum -c --strict --quiet "$overlay/post.sha256")
install -m644 "$overlay/swap-ready.hpp" "$control/src/swap-ready.hpp"
install -m755 "$overlay/tests/swap-ready.sh" "$control/tests/swap-ready.sh"
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for file in src/{backend.cpp,backend.hpp,main.cpp};do install -Dm644 "$control/$file" "$work/$file";done
(cd "$work";patch -p2 --fuzz=0 --reverse -i "$overlay/consumer.patch")
for file in src/{backend.cpp,backend.hpp,main.cpp};do cmp "$work/$file" "$overlay/legacy/$file";done
# The storage selector itself must stay byte identical to the qualified parent.
for root in "$control" "$overlay/legacy";do
 awk '/^static void apply_locked\(/{copy=1} /^static const fs::path swap_receipt=|^void swap_set\(/{copy=0} copy' "$root/src/backend.cpp" > "$work/$(basename "$root").apply"
done
cmp "$work/$(basename "$control").apply" "$work/legacy.apply"
echo 'Control28 whole-source inverses and the unchanged partition-first storage selector match the signed parent.'
