#!/usr/bin/bash
# COPR/data-only source transformation; never builds or accesses hardware.
set -Eeuo pipefail
[[ $# == 2 ]]
control=$(realpath "$1")
overlay=$(realpath "$2")
manifest=$overlay/packaging/charge-fold-source.json
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
while IFS=$'\t' read -r path digest;do
 printf '%s  %s\n' "$digest" "$control/$path" | sha256sum -c --strict --quiet -
 install -Dm644 "$control/$path" "$work/before/$path"
done < <(jq -r '.actual_control_source_units[]|[.path,.before_sha256]|@tsv' "$manifest")
git -C "$work" init -q
git -C "$control" --git-dir "$work/.git" --work-tree "$control" apply -p2 --check --whitespace=error "$overlay/packaging/charge-bridge-native91.patch"
git -C "$control" --git-dir "$work/.git" --work-tree "$control" apply -p2 --whitespace=error "$overlay/packaging/charge-bridge-native91.patch"
while IFS=$'\t' read -r path digest;do
 printf '%s  %s\n' "$digest" "$control/$path" | sha256sum -c --strict --quiet -
 install -Dm644 "$control/$path" "$work/inverse/$path"
done < <(jq -r '.actual_control_source_units[]|[.path,.after_sha256]|@tsv' "$manifest")
git -C "$work/inverse" init -q
git -C "$work/inverse" apply -p2 -R --whitespace=error "$overlay/packaging/charge-bridge-native91.patch"
while IFS= read -r path;do
 cmp "$work/before/$path" "$work/inverse/$path"
done < <(jq -r '.actual_control_source_units[].path' "$manifest")
grep -F 'uke::rollback::plan(words)' "$control/src/main.cpp" >/dev/null
grep -F '{"rollback","Read the saved kernel rollback policy"' "$control/src/cli.cpp" >/dev/null
printf 'Actual Control charge overlay: nine whole-file inverses and packaged rollback commands retained; native91 delta only,no provider or device operation.\n'
