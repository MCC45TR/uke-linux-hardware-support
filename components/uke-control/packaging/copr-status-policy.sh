#!/usr/bin/env bash
# COPR only: extend the exact installed source, retaining its speech/client layers.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
release=${2:?Unique RPM release}
[[ $release =~ ^[2-9][0-9]+$ ]]
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
mkdir -p "$output" "$stage/overlay"
curl -q -fLsS --retry 2 --max-time 180 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11267743/uke-control-2.8.1-25.src.rpm \
 -o "$stage/parent.src.rpm"
printf '%s  %s\n' 9f7fbbce88b42870d5b4f8ee71cd7678b52f867231b9f771818b19bed961276c "$stage/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
spec=$output/uke-control.spec
archive=$output/uke-control-package-update.tar.xz
(cd "$output"; find . -type f ! -name uke-control.spec ! -name uke-control-package-update.tar.xz -print0 | sort -z | xargs -0 sha256sum) > "$stage/retained"
tar -xJf "$archive" -C "$stage/overlay"
printf '%s  %s\n' 2c219562bf994f87b703011f31ecf1e64c396b4bb23e6ccd370dd22311d2d33b "$stage/overlay/src/update-status.cpp" | sha256sum -c --strict -
(cd "$stage/overlay"; find . -type f ! -path ./src/update-status.cpp ! -path ./tests/update-status.cpp -print0 | sort -z | xargs -0 sha256sum) > "$stage/overlay-retained"
for path in src/update-status.cpp tests/update-status.cpp; do install -m644 "$owner/$path" "$stage/overlay/$path"; done
(cd "$stage/overlay"; sha256sum -c --strict --quiet "$stage/overlay-retained")
tar --sort=name --owner=0 --group=0 --numeric-owner -cJf "$archive" -C "$stage/overlay" .
awk -v release="$release" '
 /^%changelog/ { print; print "* Thu Nov 05 2026 Uke contributors - 2.8.1-"release;
 print "- Report automatic staging and rollback policy in terminal-width columns."; history=1; next }
 !history {gsub(/25%[{][?]dist[}]/,release"%{?dist}")}
 /^Provides: uke-control\(update-status-v1\) = 1$/ {print;print "Provides: uke-control(update-status-checkpoint-v2) = 1";next}
 {print}
' "$spec" > "$stage/spec"
install -m644 "$stage/spec" "$spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == "uke-control-2.8.1-$release.fc46" ]]
(cd "$output"; sha256sum -c --strict --quiet "$stage/retained")
printf 'Exact installed Control source retained; status policy/layout delta prepared for release%s.\n' "$release"
