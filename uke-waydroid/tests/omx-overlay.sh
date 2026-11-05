#!/bin/sh
# SPDX-License-Identifier: MIT
# COPR only: real read-only ext4 I/O, XML preservation and overlay ownership.
set -eu
helper=${1:?fixture binary}
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT HUP INT TERM
work=$stage/state
mkdir -p "$work/images" "$stage/vendor/etc/vintf" "$stage/vendor/bin/hw" "$stage/system/system/bin/hw"
printf 'xiaomi,uke\000qcom,sm7675\000' > "$work/compatible"
printf '[waydroid]\narch=arm64_only\nvendor_type=MAINLINE\nmount_overlays=True\nimages_path=%s/images\n' "$work" > "$work/waydroid.cfg"
cat > "$stage/vendor/etc/vintf/manifest.xml" <<'EOF'
<?xml version="1.0"?>
<manifest version="1.0" type="device">
  <hal format="hidl"><name>android.hardware.media.omx</name><transport>hwbinder</transport><version>1.0</version><interface><name>IOmxStore</name><instance>default</instance></interface></hal>
  <hal format="hidl"><name>android.hardware.media.c2</name><transport>hwbinder</transport><version>1.0</version><interface><name>IComponentStore</name><instance>software</instance></interface></hal>
  <hal format="hidl"><name>android.hardware.graphics.mapper</name><transport>passthrough</transport><version>4.0</version></hal>
</manifest>
EOF
make_vendor() {
    rm -f "$work/images/vendor.img"
    mke2fs -F -q -t ext4 -d "$stage/vendor" "$work/images/vendor.img" 8192
}
make_vendor
mke2fs -F -q -t ext4 -d "$stage/system" "$work/images/system.img" 8192
sha256sum "$work/images/"*.img > "$stage/before.sha256"
target=$work/overlay/vendor/etc/vintf/manifest.xml
"$helper" "$work"
test -f "$target"
if grep -q 'android.hardware.media.omx' "$target"; then exit 1; fi
grep -q 'android.hardware.media.c2' "$target"
grep -q 'android.hardware.graphics.mapper' "$target"
sha256sum -c "$stage/before.sha256"
cp "$target" "$stage/first.xml"
"$helper" "$work"
cmp "$target" "$stage/first.xml"
# Never replace an administrator edit, even if our comment is retained.
printf '\n<!-- administrator addition -->\n' >> "$target"
cp "$target" "$stage/manual.xml"
"$helper" "$work"
cmp "$target" "$stage/manual.xml"
rm -f "$target" "$work/.uke-media-overlay.json"
"$helper" "$work"
# A newly provided service retires only the generated overlay, not the image.
touch "$stage/vendor/bin/hw/android.hardware.media.omx@1.0-service"
make_vendor
"$helper" "$work"
test ! -e "$target"
test ! -e "$work/.uke-media-overlay.json"
rm "$stage/vendor/bin/hw/android.hardware.media.omx@1.0-service"
make_vendor
"$helper" "$work"
cp "$target" "$stage/valid.xml"
# Invalid XML fails without discarding the last owned valid overlay.
printf '<manifest><hal>\n' > "$stage/vendor/etc/vintf/manifest.xml"
make_vendor
if "$helper" "$work"; then exit 1; fi
cmp "$target" "$stage/valid.xml"
# Wrong board performs no work; a linked directory is refused on Uke.
printf 'test,other\000' > "$work/compatible"
"$helper" "$work"
cmp "$target" "$stage/valid.xml"
printf 'xiaomi,uke\000' > "$work/compatible"
mv "$work/overlay/vendor/etc/vintf" "$work/overlay/vendor/etc/original"
ln -s original "$work/overlay/vendor/etc/vintf"
if "$helper" "$work"; then exit 1; fi
cmp "$work/overlay/vendor/etc/original/manifest.xml" "$stage/valid.xml"
printf 'Read-only images, retained HALs, idempotence, custom ownership, retirement and refusal checks passed\n'
