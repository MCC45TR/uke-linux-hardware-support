#!/usr/bin/bash
# COPR source worker: append a rule to the exact accepted self-contained216.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11299926/uke-control-2.16.0-1.src.rpm -o "$work/parent.rpm"
printf '%s  %s\n' c05d1472413aba357022c8b18b38d88305f8e9b690a9dcc26e592ceb819f8744 "$work/parent.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/parent.rpm") == uke-control-2.16.0-1 ]]
rpm2cpio "$work/parent.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
(cd "$owner";jq -r '.source_assets[]|.sha256+"  "+.path' packaging/native-proximity-source.json | sha256sum -c --strict --quiet -)
install -m644 "$owner/packaging/89-uke-native-proximity.rules" "$owner/docs/NATIVE-PROXIMITY-SELECTION.md" "$owner/packaging/fold-native-proximity-spec.sh" "$owner/packaging/copr-control-native-proximity.sh" "$owner/packaging/native-proximity-source.json" "$output/"
bash "$owner/packaging/fold-native-proximity-spec.sh" "$output/uke-control.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.16.0-2.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
rpmspec -q --requires "$output/uke-control.spec" > "$work/requires"
awk '/^uke-control(-tui|-client)?(\([^)]*\))? = /{if($NF!="2.16.0-2.fc46")bad=1;n++}END{exit bad||n<5}' "$work/requires"
rpmspec -q --provides "$output/uke-control.spec" > "$work/provides"
grep -Fx 'uke-control(native-proximity-selection-v1) = 1' "$work/provides"
grep -Fx 'uke-nfc-ir = 1.1.0-3.fc46' "$work/provides"
printf 'Control216r2 native proximity source: exact admitted216 inputs and new versioned classification; existing names and matching TUI preserved.\n'
