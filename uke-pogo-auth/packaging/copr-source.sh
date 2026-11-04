#!/usr/bin/env bash
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
owner=$(cd -- "$here/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
root=$stage/uke-pogo-auth-0.1
mkdir -p "$output" "$root/upstream"
git -C "$owner" archive HEAD uke-pogo-auth uke-cdsp-pogo-firmware/src uke-boot/src/oem-video.hpp | tar -xf - -C "$root"
while IFS=$'\t' read -r name url hash; do
    curl -fLsS --max-time 120 --max-filesize 16777216 --proto '=https' --proto-redir '=https' "$url" -o "$output/$name.tar.gz"
    printf '%s  %s\n' "$hash" "$output/$name.tar.gz" | sha256sum -c --strict
    tar -tzf "$output/$name.tar.gz" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
    mkdir -p "$root/upstream/$name"
    tar -xzf "$output/$name.tar.gz" --strip-components=1 -C "$root/upstream/$name"
done < <(jq -r '.sources[]|[.name,.url,.sha256]|@tsv' "$here/manifests/upstreams.json")
for compiler in g++ clang++; do
    CXX=$compiler bash "$root/uke-pogo-auth/tests/check.sh" "$root/upstream/minkipc"
done
find "$root/upstream" -depth -delete
tar --sort=name --mtime="@$(git -C "$owner" show -s --format=%ct HEAD)" --owner=0 --group=0 --numeric-owner \
    -czf "$output/uke-pogo-auth-0.1.tar.gz" -C "$stage" uke-pogo-auth-0.1
install -m644 "$here/packaging/uke-pogo-auth.spec" "$output/uke-pogo-auth.spec"
rpmbuild -bs --define "_sourcedir $output" --define "_srcrpmdir $output" "$output/uke-pogo-auth.spec"
