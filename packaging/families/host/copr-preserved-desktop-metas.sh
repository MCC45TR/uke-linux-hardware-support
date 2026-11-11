#!/usr/bin/bash
# Source worker only. No native build or successful-NVR republication.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
workspace=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
contract=$workspace/packaging/families/consolidation/desktop-metas-source-retention.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --max-time 40 'https://copr.fedorainfracloud.org/api_3/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&packagename=uke-desktop-metas&limit=1000' > "$work/history.json"
version=$(jq -er .output_version "$contract")
if ! jq --arg version "$version" -e '(.items|length)<1000 and all(.items[];.state!="succeeded" or (.source_package.version!=$version and .source_package.version!=($version+".fc46")))' "$work/history.json" >/dev/null;then
 printf 'Desktop meta version is already published;advance its reviewed source before a new build.\n' >&2
 exit 1
fi
reference=$(jq -er .reference_commit "$contract")
git -C "$work" init -q reference
git -C "$work/reference" remote add origin https://github.com/MCC45TR/uke-linux-hardware-support.git
git -C "$work/reference" fetch -q --depth=1 origin "$reference"
git -C "$work/reference" checkout -q --detach FETCH_HEAD
[[ $(git -C "$work/reference" rev-parse HEAD) == "$reference" ]]
bash "$workspace/packaging/families/host/seed-desktop-metas-source.sh" "$work/reference/packaging/families/control-gui" "$contract"
make -C "$work/reference/packaging/families/control-gui-metas" srpm outdir="$work/stage"
mapfile -t sources < <(find "$work/stage" -maxdepth 1 -type f -name '*.src.rpm')
[[ ${#sources[@]} == 1 ]]
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "${sources[0]}") == "uke-desktop-metas-$version.fc46" ]]
rpm2cpio "${sources[0]}" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output"
rpm2cpio "${sources[0]}" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
printf 'Desktop meta source retained from exact historical code and signed input;no runtime code or package identity changed.\n'
