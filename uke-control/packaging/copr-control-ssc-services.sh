#!/usr/bin/bash
# COPR source worker only. Preserve Control5 and the installed SSC32 provider.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/ssc-services.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output && -z $(git -C "$workspace" status --porcelain) ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/ssc" "$work/native" "$work/overlay"
download(){
 curl -qfsSL --retry 2 --max-time 120 "$(jq -er ".$1.url" "$contract")" -o "$work/$1.rpm"
 printf '%s  %s\n' "$(jq -er ".$1.srpm_sha256" "$contract")" "$work/$1.rpm" | sha256sum -c --strict --quiet -
 rpm2cpio "$work/$1.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
}
download control
rpm2cpio "$work/control.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 60 ]]
(cd "$output";find . -type f ! -name uke-control.spec ! -name uke-control-ssc-services.tar.xz -print0 | sort -z | xargs -0 sha256sum > "$work/parents.sha256")
download ssc
rpm2cpio "$work/ssc.rpm" | (cd "$work/ssc";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$work/ssc" -maxdepth 1 -type f | wc -l) == 2 ]]
bash "$owner/packaging/prepare-ssc-sources.sh" "$work/ssc/uke-sensor-integration-1.0.0.tar.xz" "$work/overlay/inputs" "$contract"
download native
rpm2cpio "$work/native.rpm" | (cd "$work/native";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$work/native" -maxdepth 1 -type f | wc -l) == 4 ]]
jq -r '.native.archives[]|.sha256+"  "+.filename' "$contract" | (cd "$work/native";sha256sum -c --strict --quiet -)
for archive in "$work/native/"*.tar.xz;do
 destination=$output/$(basename "$archive")
 cmp "$archive" "$destination"
done
install -m644 "$owner/packaging/ssc-unit-state.sh" "$work/overlay/ssc-unit-state.sh"
install -m644 "$owner/tests/ssc-refresh.sh" "$work/overlay/ssc-refresh-test.sh"
install -m644 "$owner/tests/ssc-unit-state.sh" "$work/overlay/ssc-unit-state-test.sh"
install -m644 "$owner/docs/SSC-SERVICES.md" "$work/overlay/SSC-SERVICES.md"
install -m644 "$work/native/uke-kernel-migration-check.spec" "$work/overlay/native102.spec"
jq --arg commit "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" \
 '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/overlay/ssc-services.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-ssc-services.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-ssc-spec.mjs" "$output/uke-control.spec" "$contract" "$owner/packaging/ssc-unit-state.sh" "$owner/packaging/ssc-recipe.json" "$work/native/uke-kernel-migration-check.spec"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
(cd "$output";sha256sum -c --strict --quiet "$work/parents.sha256")
bash "$owner/tests/ssc-unit-state.sh" --data-only "$output/uke-control.spec"
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-6.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control6:59 unchanged Control5 sources,SSC32 inputs and native102 archives preserved;three existing output names only.\n'
