#!/usr/bin/bash
# Data-only source/recipe regression; native builds remain in COPR.
set -Eeuo pipefail
workspace=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
parent=$(realpath "${1:?Actual GUI6 spec}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cp "$parent" "$work/good.spec"
bash "$workspace/packaging/families/host/apply-control-qt-runtime.sh" "$work/good.spec"
for mutation in version tests source;do
 cp "$parent" "$work/$mutation.spec"
 case $mutation in
  version) sed -i 's/^Version:.*/Version: 1.6.1/' "$work/$mutation.spec";;
  tests) sed -i 's/^%ctest$/true/' "$work/$mutation.spec";;
  source) printf '\n# altered source recipe\n' >> "$work/$mutation.spec";;
 esac
 if bash "$workspace/packaging/families/host/apply-control-qt-runtime.sh" "$work/$mutation.spec" > "$work/$mutation.log" 2>&1;then
  printf 'Qt recipe admitted changed parent:%s\n' "$mutation" >&2;exit 1
 fi
done
if bash "$workspace/packaging/families/host/apply-control-qt-runtime.sh" "$work/good.spec" > "$work/reentry.log" 2>&1;then
 echo 'Qt recipe admitted a repeated release rewrite' >&2;exit 1
fi
printf 'Qt runtime recipe data checks: whole inverse,CTest retained,four changed/repeated-input refusals;no native compilation.\n'
