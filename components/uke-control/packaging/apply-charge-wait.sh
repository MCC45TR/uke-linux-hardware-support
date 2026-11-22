#!/usr/bin/bash
# Data-only overlay over the actual Control217 helper.
set -Eeuo pipefail
[[ $# == 2 ]]
runtime=$(realpath "$1");overlay=$(realpath "$2")
contract=$overlay/charge-wait-source.json
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
file=src/battery/control-main.cpp
printf '%s  %s\n' "$(jq -er .source_unit.before_sha256 "$contract")" "$runtime/$file" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cp -a "$runtime" "$work/before"
git -c init.defaultBranch=senemos -C "$work" init -q
git -C "$runtime" --git-dir "$work/.git" --work-tree "$runtime" apply --check --whitespace=error "$overlay/charge-wait.patch"
git -C "$runtime" --git-dir "$work/.git" --work-tree "$runtime" apply --whitespace=error "$overlay/charge-wait.patch"
printf '%s  %s\n' "$(jq -er .source_unit.after_sha256 "$contract")" "$runtime/$file" | sha256sum -c --strict --quiet -
cp -a "$runtime" "$work/inverse"
git -C "$work/inverse" --git-dir "$work/.git" --work-tree "$work/inverse" apply -R --whitespace=error "$overlay/charge-wait.patch"
diff -qr "$work/before" "$work/inverse"
install -m644 "$overlay/charge-wait.cpp" "$runtime/tests/charge-wait.cpp"
printf 'Actual217 helper wait overlay: one changed source unit,complete runtime inverse and unchanged hardware/control bodies.\n'
