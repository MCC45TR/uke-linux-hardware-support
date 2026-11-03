#!/usr/bin/env bash
# COPR only: publish the reviewed supplier observer without replacing charging policy.
set -Eeuo pipefail
trap 'printf "Battery source composition failed at line %s (status %s)\n" "$LINENO" "$?" >&2' ERR
export LC_ALL=C
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
[[ ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
parent=2a225b5c158cd61eb7cc58c3655bc395b2afba8bf9a6f71e1997e4139ebc63cc
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11249690/uke-control-2.8.1-7.fc46.src.rpm \
 -o "$stage/parent.src.rpm"
printf '%s  %s\n' "$parent" "$stage/parent.src.rpm" | sha256sum -c -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-control-2.8.1-7.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
mkdir -p "$stage/outer" "$stage/power" "$stage/battery" "$output"
(cd "$stage/outer"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
unpack() {
    tar -tJf "$1" > "$stage/tar.entries"
    awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/tar.entries"
    tar -xJf "$1" -C "$2"
}
unpack "$stage/outer/uke-control-2.8.1.tar.xz" "$stage/outer"
control=$stage/outer/uke-control-2.8.1
(cd "$control"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
unpack "$control/inputs/uke-power-integration/uke-power-integration-1.0.0.tar.xz" "$stage/power"
power=$stage/power/uke-power-integration-1.0.0
(cd "$power"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$power/inputs/uke-battery-observer
unpack "$input/uke-battery-observer-1.4.1.tar.xz" "$stage/battery"
battery=$stage/battery/uke-battery-observer-1.4.1
(cd "$battery"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
cp "$battery/SOURCE-SHA256SUMS" "$stage/before.sha256"
cat > "$stage/overlays" <<'FILES'
Makefile
README.md
src/battery/collector.cpp
src/battery/pack-variant.hpp
tests/battery-collector.sh
tests/battery-pack-variant.cpp
FILES
while read -r path; do install -Dm644 "$owner/uke-platform-runtime/$path" "$battery/$path"; done < "$stage/overlays"
while read -r hash path; do
    path=${path#./}
    if ! grep -Fxq "$path" "$stage/overlays"; then
        printf '%s  %s\n' "$hash" "$battery/$path" | sha256sum -c --quiet -
    fi
done < "$stage/before.sha256"
jq -Rn '[inputs]' < "$stage/overlays" > "$stage/overlays.json"
jq -n --arg parent "$parent" --arg commit "$(git -C "$owner" rev-parse HEAD)" \
 --slurpfile overlays "$stage/overlays.json" \
 '{schema:1,source_commit:$commit,parent_copr_build:11249690,parent_srpm_sha256:$parent,
 version:"1.4.1-7.fc46",overlays:$overlays[0],firmware_payload_included:false,
 charging_policy_changed:false,physical_acceptance:false}' > "$battery/pack-variant-source.json"
(cd "$battery"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
tar --sort=name --mtime="@$(git -C "$owner" show -s --format=%ct HEAD)" --owner=0 --group=0 --numeric-owner \
 -cJf "$output/uke-battery-observer-1.4.1.tar.xz" -C "$stage/battery" uke-battery-observer-1.4.1
install -m644 "$input/uke-battery.conf" "$output/"
[[ $(grep -c '^Release:' "$input/uke-battery-observer.spec") == 1 ]]
[[ $(grep -c '^Requires:.*libstdc++%{?_isa} >= 16.2.1' "$input/uke-battery-observer.spec") == 3 ]]
sed -e 's/^Release: 1%{?dist}$/Release: 7%{?dist}/' \
 -e '/^%doc.*SOURCE-SHA256SUMS/s/$/ pack-variant-source.json/' \
 "$input/uke-battery-observer.spec" > "$output/uke-battery-observer.spec"
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-battery-observer.spec") == 1.4.1-7.fc46 ]]
