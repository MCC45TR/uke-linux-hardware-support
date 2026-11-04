#!/bin/sh
# COPR-only fixtures: preserve unrelated properties and remain idempotent.
set -eu
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT HUP INT TERM
mkdir "$stage/state"
printf '[waydroid]\narch = arm64_only\n\n[properties]\nro.hardware.egl = mesa\npersist.waydroid.multi_windows = false\n\n[other]\nkeep = yes\n' > "$stage/state/waydroid.cfg"
printf 'ro.hardware.egl=mesa\nro.hardware.gralloc=gbm\n' > "$stage/state/waydroid_base.prop"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf"
grep -qx 'persist.waydroid.multi_windows = true' "$stage/state/waydroid.cfg"
grep -qx 'persist.waydroid.multi_windows=true' "$stage/state/waydroid_base.prop"
grep -qx 'ro.hardware.egl = mesa' "$stage/state/waydroid.cfg"
grep -qx 'ro.hardware.gralloc=gbm' "$stage/state/waydroid_base.prop"
grep -qx 'keep = yes' "$stage/state/waydroid.cfg"
cp "$stage/state/waydroid.cfg" "$stage/first.cfg"
cp "$stage/state/waydroid_base.prop" "$stage/first.prop"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf"
cmp "$stage/first.cfg" "$stage/state/waydroid.cfg"
cmp "$stage/first.prop" "$stage/state/waydroid_base.prop"
# A fresh properties section and an administrator preference also work.
printf '[waydroid]\narch = arm64_only\n' > "$stage/state/waydroid.cfg"
printf 'persist.waydroid.multi_windows=false\n' > "$stage/defaults"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$stage/defaults"
grep -qx '\[properties\]' "$stage/state/waydroid.cfg"
grep -qx 'persist.waydroid.multi_windows = false' "$stage/state/waydroid.cfg"
# Invalid preferences fail without mutating state.
cp "$stage/state/waydroid.cfg" "$stage/unchanged.cfg"
printf 'persist.waydroid.multi_windows=invalid\n' > "$stage/defaults"
if sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$stage/defaults"; then
    exit 1
fi
cmp "$stage/unchanged.cfg" "$stage/state/waydroid.cfg"
