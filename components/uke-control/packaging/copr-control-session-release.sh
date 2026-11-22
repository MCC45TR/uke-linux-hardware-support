#!/usr/bin/bash
# Source worker only: publish the self-contained qualified215r2 source cohort.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output && ! -L $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable:custom:kernel-migration-20261105/srpm-builds/11293633/uke-control-2.15.0-2.src.rpm -o "$work/qualified.rpm"
printf '%s  %s\n' 8cdc9b452eacc43860eb458e19aac8601230782e7572a8158b021ffe719d9a01 "$work/qualified.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/qualified.rpm") == uke-control-2.15.0-2 ]]
rpm2cpio "$work/qualified.rpm" > "$work/qualified.cpio"
cpio -t --quiet < "$work/qualified.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/qualified.cpio")
printf '%s  %s\n' 2637d49b80c10f6e57dde9e36a437a657d08d83edfa1612f3848a1ed264c5a14 "$output/uke-control.spec" | sha256sum -c --strict --quiet -
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.15.0-2.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control215r2 release uses exact self-contained qualified SRPM/spec; no legacy runtime producer or source constructor is invoked.\n'
