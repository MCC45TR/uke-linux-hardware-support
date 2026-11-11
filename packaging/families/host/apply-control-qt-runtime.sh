#!/usr/bin/bash
# Data-only, whole-spec inverse. No compiler, package transaction or service.
set -Eeuo pipefail
[[ $# == 1 ]]
workspace=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
spec=$(realpath "${1:?Actual GUI6 spec}")
contract=$workspace/packaging/families/consolidation/control-qt-runtime.json
patch=$workspace/packaging/families/host/control-qt-runtime.patch
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$spec" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .patch_sha256 "$contract")" "$patch" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cp "$spec" "$work/uke-control-qt.spec"
git -c init.defaultBranch=fixture init -q "$work"
git -C "$work" apply --check --whitespace=error "$patch"
git -C "$work" apply --whitespace=error "$patch"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$work/uke-control-qt.spec" | sha256sum -c --strict --quiet -
cp "$work/uke-control-qt.spec" "$work/reviewed.spec"
git -C "$work" apply -R --check --whitespace=error "$patch"
git -C "$work" apply -R --whitespace=error "$patch"
cmp "$spec" "$work/uke-control-qt.spec"
[[ $(grep -Fxc '%cmake -DBUILD_TESTING=ON' "$work/reviewed.spec") == 1 ]]
[[ $(grep -Fxc '%ctest' "$work/reviewed.spec") == 1 ]]
! grep -Eq '^%package tests|^%files tests' "$work/reviewed.spec"
install -m644 "$work/reviewed.spec" "$spec"
printf 'Qt runtime recipe: complete parent inverse and retained native CTest; two runtime outputs, no test/debug RPM.\n'
