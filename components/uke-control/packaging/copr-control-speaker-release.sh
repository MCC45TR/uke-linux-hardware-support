#!/usr/bin/bash
# Source worker only: publish the self-contained qualified214 source cohort.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output && ! -L $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable:custom:kernel-migration-20261105/srpm-builds/11292369/uke-control-2.14.0-1.src.rpm -o "$work/qualified.rpm"
printf '%s  %s\n' a20418325d53b6d2cf9e11684d2b24af3eb112cd64a29334ed81c5f492506f40 "$work/qualified.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/qualified.rpm") == uke-control-2.14.0-1 ]]
rpm2cpio "$work/qualified.rpm" > "$work/qualified.cpio"
cpio -t --quiet < "$work/qualified.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/qualified.cpio")
printf '%s  %s\n' 7e2641a1521e99d10371cb1c2cd01f0cf4aa66e8c676a0b70a15315afdeb0a03 "$output/uke-control.spec" | sha256sum -c --strict --quiet -
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.14.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control214 release uses exact self-contained qualified SRPM/spec; no legacy runtime producer or source constructor is invoked.\n'
