#!/usr/bin/env bash
# Native COPR source worker; retains private-stock acquisition/publication gates.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
stage=$(mktemp -d);trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$stage/parent" "$output"
curl -q -fLsS --retry 2 --max-time 120 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11263798/uke-cdsp-pogo-firmware-0.2-8.src.rpm -o "$stage/parent.src.rpm"
printf '%s  %s\n' a35be1f160796c6f9817256824568e0db30912f76221ee0105a0a0d315b3d576 "$stage/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$stage/parent.src.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
(cd "$stage/parent";cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
cp "$stage/parent/uke-cdsp-pogo-firmware-0.2.tar.gz" "$output/"
install -m644 "$owner/packaging/unstable-release.hpp" "$owner/packaging/managed-firmware-version.patch" "$output/"
awk '
 /^Release:/ {$0="Release: 9%{?dist}"}
 /^Source0:/ {print;print "Source1: unstable-release.hpp\nSource2: managed-firmware-version.patch\nBuildRequires: patch";next}
 /^%setup -q$/ {print;print "install -m644 %{SOURCE1} uke-cdsp-pogo-firmware/src/unstable-release.hpp\npatch --batch --fuzz=0 -p1 < %{SOURCE2}";next}
 {print}' "$stage/parent/uke-cdsp-pogo-firmware.spec" > "$output/uke-cdsp-pogo-firmware.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-cdsp-pogo-firmware.spec") == uke-cdsp-pogo-firmware-0.2-9.fc46 ]]
tar -xzf "$output/uke-cdsp-pogo-firmware-0.2.tar.gz" -C "$stage"
(cd "$stage/uke-cdsp-pogo-firmware-0.2";git apply --check "$output/managed-firmware-version.patch")
cmp "$stage/parent/uke-cdsp-pogo-firmware-0.2.tar.gz" "$output/uke-cdsp-pogo-firmware-0.2.tar.gz"
printf 'Original firmware8 archive and all native controls retained; canonical unstable parser and complete semantic target selection prepared.\n'
