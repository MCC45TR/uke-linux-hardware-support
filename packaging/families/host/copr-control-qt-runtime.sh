#!/usr/bin/bash
# COPR source worker: consume archived exact GUI6 sources without rebuilding Qt.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
workspace=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
contract=$workspace/packaging/families/consolidation/control-qt-runtime.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 120 "$(jq -er .parent.archive_url "$contract")" -o "$work/source.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/source.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/source.rpm") == uke-control-gui-qt-1.6.0-6 ]]
rpm2cpio "$work/source.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output"
rpm2cpio "$work/source.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f | wc -l) == 2 ]]
printf '%s  %s\n' "$(jq -er .parent.source_archive_sha256 "$contract")" "$output/uke-control-qt-1.6.0.tar.gz" | sha256sum -c --strict --quiet -
bash "$workspace/packaging/families/host/apply-control-qt-runtime.sh" "$output/uke-control-qt.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control-qt.spec") == uke-control-gui-qt-1.6.0-7.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control-qt.spec" | sort > "$work/outputs"
printf '%s\n' uke-control-gui-qt uke-waydroid-installer > "$work/expected"
cmp "$work/outputs" "$work/expected"
printf 'Qt source1.6.0-7: exact GUI/installer source archive,all CTest gates retained; runtime test/debug outputs retired.\n'
