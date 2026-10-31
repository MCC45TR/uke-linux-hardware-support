#!/usr/bin/env bash
# COPR-only: produce the existing standalone audio-data package.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
root=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
bash "$root/packaging/families/host/standalone.sh" uke-displayport-audio "$work/source"
mapfile -t sources < <(find "$work/source" -maxdepth 1 -type f -name '*.src.rpm')
[[ ${#sources[@]} == 1 ]]
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "${sources[0]}") == uke-displayport-audio-0.1.0-9.fc46 ]]
rpm2cpio "${sources[0]}" > "$work/source.cpio"
cpio -t --quiet < "$work/source.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$work/source.cpio")
