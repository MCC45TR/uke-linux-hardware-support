#!/usr/bin/bash
# COPR source worker only: retain both actual immutable firmware source archives.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
workspace=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
contract=$workspace/packaging/families/consolidation/firmware-consolidation.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for name in firmware speaker;do
 curl -qfsSL --retry 2 --max-time 180 "$(jq -er ".$name.url" "$contract")" -o "$work/$name.rpm"
 printf '%s  %s\n' "$(jq -er ".$name.srpm_sha256" "$contract")" "$work/$name.rpm" | sha256sum -c --strict --quiet -
 [[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/$name.rpm") == "$(jq -er ".$name.nvr" "$contract")" ]]
 rpm2cpio "$work/$name.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
done
mkdir "$output" "$work/speaker"
rpm2cpio "$work/firmware.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
rpm2cpio "$work/speaker.rpm" | (cd "$work/speaker";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f | wc -l) == 2 ]]
for name in firmware speaker;do
 if [[ $name == firmware ]];then archive=$output/uke-firmware-3.0.304.0.tar.xz;else archive=$work/speaker/uke-speaker-firmware-3.0.304.0.tar.xz;fi
 printf '%s  %s\n' "$(jq -er ".$name.archive_sha256" "$contract")" "$archive" | sha256sum -c --strict --quiet -
done
install -m644 "$work/speaker/uke-speaker-firmware-3.0.304.0.tar.xz" "$output/retained-speaker-firmware-3.0.304.0-2.tar.xz"
node "$workspace/packaging/families/host/fold-speaker-firmware.mjs" "$output/uke-firmware.spec"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-firmware.spec" | sha256sum -c --strict --quiet -
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-firmware.spec") == uke-firmware-3.0.304.0-15.fc46 ]]
[[ $(rpmspec -q --qf '%{NAME}' "$output/uke-firmware.spec") == uke-firmware ]]
printf 'Firmware15 source:both original data archives,one existing runtime RPM,no service or executable addition.\n'
