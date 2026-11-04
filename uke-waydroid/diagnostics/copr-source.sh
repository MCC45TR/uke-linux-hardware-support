#!/usr/bin/env bash
# COPR source worker: retain only open NDK sysroot headers/stubs and notices.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/unpacked" "$stage/source/uke-waydroid-diagnostics-0.1.0"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://dl.google.com/android/repository/android-ndk-r27d-linux.zip -o "$stage/ndk.zip"
[[ $(stat -c %s "$stage/ndk.zip") == 663956036 ]]
printf '%s  %s\n' 22105e410cf29afcf163760cc95522b9fb981121 "$stage/ndk.zip" | sha1sum -c --strict
sha256sum "$stage/ndk.zip"
unzip -q "$stage/ndk.zip" \
 'android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/*' \
 'android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/30/*' \
 'android-ndk-r27d/NOTICE' 'android-ndk-r27d/source.properties' -d "$stage/unpacked"
source=$stage/source/uke-waydroid-diagnostics-0.1.0
mv "$stage/unpacked/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/sysroot" "$source/sysroot"
install -m644 "$stage/unpacked/android-ndk-r27d/NOTICE" "$source/NDK-NOTICE"
install -m644 "$stage/unpacked/android-ndk-r27d/source.properties" "$source/ndk-source.properties"
install -m644 "$owner/uke-waydroid/diagnostics/vulkan-check.cpp" "$source/vulkan-check.cpp"
install -m644 "$owner/uke-waydroid/LICENSE" "$source/LICENSE"
git -C "$owner" rev-parse HEAD > "$source/source-commit"
tar -cJf "$output/uke-waydroid-diagnostics-0.1.0.tar.xz" -C "$stage/source" uke-waydroid-diagnostics-0.1.0
install -m644 "$owner/uke-waydroid/diagnostics/uke-waydroid-diagnostics.spec" "$output/uke-waydroid-diagnostics.spec"
