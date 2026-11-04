#!/usr/bin/env bash
set -Eeuo pipefail
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
owner=$(cd -- "$here/.." && pwd)
out=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'rm -r -- "$stage"' EXIT
mkdir -p "$out" "$stage/uke-cdsp-pogo-firmware-0.2"
root=$stage/uke-cdsp-pogo-firmware-0.2
git -C "$owner" archive HEAD uke-cdsp-pogo-firmware uke-boot/src/oem-video.hpp | tar -xf - -C "$root"
for compiler in g++ clang++; do
    CXX=$compiler bash "$root/uke-cdsp-pogo-firmware/tests/check.sh"
done
tar --sort=name --mtime="@$(git -C "$owner" show -s --format=%ct HEAD)" --owner=0 --group=0 --numeric-owner \
    -czf "$out/uke-cdsp-pogo-firmware-0.2.tar.gz" -C "$stage" uke-cdsp-pogo-firmware-0.2
install -m644 "$here/packaging/uke-cdsp-pogo-firmware.spec" "$out/uke-cdsp-pogo-firmware.spec"
rpmbuild -bs --define "_sourcedir $out" --define "_srcrpmdir $out" "$out/uke-cdsp-pogo-firmware.spec"
