#!/bin/sh
# COPR-only fixtures: preserve unrelated properties and remain idempotent.
set -eu
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT HUP INT TERM
mkdir "$stage/state"
printf 'test,other\000' > "$stage/compatible"
printf '[waydroid]\narch = arm64_only\n\n[properties]\nro.hardware.egl = mesa\npersist.waydroid.multi_windows = false\n\n[other]\nkeep = yes\n' > "$stage/state/waydroid.cfg"
printf 'ro.hardware.egl=mesa\nro.hardware.gralloc=gbm\n' > "$stage/state/waydroid_base.prop"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
grep -qx 'persist.waydroid.multi_windows = true' "$stage/state/waydroid.cfg"
grep -qx 'persist.waydroid.multi_windows=true' "$stage/state/waydroid_base.prop"
grep -qx 'ro.hardware.egl = mesa' "$stage/state/waydroid.cfg"
grep -qx 'ro.hardware.gralloc=gbm' "$stage/state/waydroid_base.prop"
grep -qx 'keep = yes' "$stage/state/waydroid.cfg"
cp "$stage/state/waydroid.cfg" "$stage/first.cfg"
cp "$stage/state/waydroid_base.prop" "$stage/first.prop"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
cmp "$stage/first.cfg" "$stage/state/waydroid.cfg"
cmp "$stage/first.prop" "$stage/state/waydroid_base.prop"
# A fresh properties section and an administrator preference also work.
printf '[waydroid]\narch = arm64_only\n' > "$stage/state/waydroid.cfg"
printf 'persist.waydroid.multi_windows=false\n' > "$stage/defaults"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$stage/defaults" "$stage/compatible"
grep -qx '\[properties\]' "$stage/state/waydroid.cfg"
grep -qx 'persist.waydroid.multi_windows = false' "$stage/state/waydroid.cfg"
# Invalid preferences fail without mutating state.
cp "$stage/state/waydroid.cfg" "$stage/unchanged.cfg"
printf 'persist.waydroid.multi_windows=invalid\n' > "$stage/defaults"
if sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$stage/defaults" "$stage/compatible"; then
    exit 1
fi
cmp "$stage/unchanged.cfg" "$stage/state/waydroid.cfg"
# Only the exact Uke compatible receives the hardware property correction.
if grep -q 'mesa.fd.dev.features' "$stage/state/waydroid.cfg"; then exit 1; fi
printf 'xiaomi,uke\000qcom,sm7675\000' > "$stage/compatible"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
grep -qx 'mesa.fd.dev.features = enable_tp_ubwc_flag_hint=0' "$stage/state/waydroid.cfg"
grep -qx 'mesa.fd.dev.features=enable_tp_ubwc_flag_hint=0' "$stage/state/waydroid_base.prop"
grep -qx 'ro.hardware.vulkan=freedreno' "$stage/state/waydroid_base.prop"
grep -qx 'ro.hardware.gralloc=gbm' "$stage/state/waydroid_base.prop"
cp "$stage/state/waydroid.cfg" "$stage/uke.cfg"
cp "$stage/state/waydroid_base.prop" "$stage/uke.prop"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
cmp "$stage/uke.cfg" "$stage/state/waydroid.cfg"
cmp "$stage/uke.prop" "$stage/state/waydroid_base.prop"
# Codec2 is selected only for the exact Uke ARM64-only MAINLINE image pair.
printf '[waydroid]\narch=arm64_only\nvendor_type=MAINLINE\n\n[properties]\nkeep = yes\n' > "$stage/state/waydroid.cfg"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
grep -qx 'debug.stagefright.ccodec = 1' "$stage/state/waydroid.cfg"
grep -qx 'debug.stagefright.ccodec=1' "$stage/state/waydroid_base.prop"
grep -qx 'keep = yes' "$stage/state/waydroid.cfg"
cp "$stage/state/waydroid.cfg" "$stage/codec.cfg"
sh "$owner/runtime/uke-waydroid-prepare" "$stage/state" "$owner/config/uke-defaults.conf" "$stage/compatible"
cmp "$stage/codec.cfg" "$stage/state/waydroid.cfg"
