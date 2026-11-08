#!/usr/bin/bash
# Source worker only: preserve the existing producer, build one policy client.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
root=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
output=$(realpath -m "${1:?COPR source result}")
[[ ! -e $output ]]
cd "$root"
(
    cd packaging/families
    for check in split-spec filelists contracts cohort; do
        bash "tests/$check.sh"
    done
)
FEDORA_TARGET=rawhide bash packaging/families/host/compose.sh uke-audio-integration "$PWD/packaging/families/build/audio-current"
mapfile -t inputs < <(find packaging/families/build/audio-current/SRPMS -maxdepth 1 -type f -name '*.src.rpm')
[[ ${#inputs[@]} == 1 ]]
identity=$(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "${inputs[0]}")
printf 'Prepared audio source identity:%s\n' "$identity"
[[ $identity == uke-audio-integration-1.1.0-1.fc46 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
rpm2cpio "${inputs[0]}" > "$work/source.cpio"
cpio -t --quiet < "$work/source.cpio" > "$work/files"
awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}' "$work/files"
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$work/source.cpio")
mapfile -t specs < <(find "$output" -maxdepth 1 -type f -name '*.spec')
[[ ${#specs[@]} == 1 ]]
# rpmspec also enumerates the source-only parent, which has no runtime file
# list. Check actual output declarations here; native RPM results are checked
# independently after the build.
awk '/^%files/{if($0!="%files -n uke-speaker-orientation")bad=1;files++}END{exit bad || files!=1}' "${specs[0]}"
printf 'Audio source family: one current orientation RPM; no regenerated legacy policy/control RPM, no new producer or desktop fork.\n'
