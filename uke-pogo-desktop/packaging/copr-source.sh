#!/usr/bin/env bash
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
owner=$(cd -- "$here/.." && pwd)
out=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$out"
git -C "$owner" archive --prefix=uke-pogo-desktop-0.1/ HEAD uke-pogo-desktop | tar -xf - -C "$stage"
tar --sort=name --mtime="@$(git -C "$owner" show -s --format=%ct HEAD)" --owner=0 --group=0 --numeric-owner \
 -czf "$out/uke-pogo-desktop-0.1.tar.gz" --transform='s@^uke-pogo-desktop-0.1/uke-pogo-desktop@uke-pogo-desktop-0.1@' \
 -C "$stage" uke-pogo-desktop-0.1/uke-pogo-desktop
install -m644 "$here/packaging/uke-pogo-desktop.spec" "$out/uke-pogo-desktop.spec"
rpmbuild -bs --define "_sourcedir $out" --define "_srcrpmdir $out" "$out/uke-pogo-desktop.spec"
