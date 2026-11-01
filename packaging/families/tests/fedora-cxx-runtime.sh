#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../..";pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
for record in uke-core-meta:1 uke-boot-integration:4 uke-battery-observer:3; do
    name=${record%:*};count=${record#*:}
    printf 'Name: %s\n' "$name" > "$stage/in"
    for ((i=0;i<count;i++));do printf '%s\n' 'Requires: systemd senemos-native-runtime(libstdc++) openssl-libs' >> "$stage/in";done
    printf '%s\n' '%post' 'echo preserved' >> "$stage/in"
    bash "$root/packaging/families/host/fedora-cxx-runtime.sh" "$stage/in" "$stage/out"
    grep -Fx 'echo preserved' "$stage/out" >/dev/null
    if bash "$root/packaging/families/host/fedora-cxx-runtime.sh" "$stage/out" "$stage/duplicate";then exit 1;fi
done
printf '%s\n' 'Name: unrelated' 'Requires: senemos-native-runtime(libstdc++)' > "$stage/in"
if bash "$root/packaging/families/host/fedora-cxx-runtime.sh" "$stage/in" "$stage/out";then exit 1;fi
echo 'PASS: reviewed GNU consumers keep an ISA/version dependency; unknown and repeated overlays refuse.'
