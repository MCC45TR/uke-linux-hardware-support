#!/usr/bin/bash
# COPR source worker only. Reuse all actual Control3 sources unchanged.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/adaptive-services.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
[[ -z $(git -C "$workspace" status --porcelain) ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 "$(jq -er .control.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .control.srpm_sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/parent.rpm") == uke-control-2.17.0-3 ]]
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$work/overlay"
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 55 ]]
printf '%s  %s\n' "$(jq -er .archives.source0 "$contract")" "$output/uke-control-2.8.1.tar.xz" | sha256sum -c --strict --quiet -
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parents.sha256")
mkdir "$work/source0"
tar -xJf "$output/uke-control-2.8.1.tar.xz" --strip-components=1 -C "$work/source0"
bash "$owner/tests/adaptive-services-data.sh" "$output/uke-control.spec" "$work/source0" "$work/source-data-check"
for item in prepare-adaptive-sources.sh check-adaptive-plugin-deps.sh adaptive-unit-state.sh;do
 install -m644 "$owner/packaging/$item" "$work/overlay/$item"
done
install -m644 "$owner/tests/adaptive-unit-state.sh" "$work/overlay/adaptive-unit-state-test.sh"
install -m644 "$owner/docs/ADAPTIVE-SERVICES.md" "$work/overlay/ADAPTIVE-SERVICES.md"
jq --arg commit "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" \
 '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/overlay/adaptive-services.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-adaptive-services.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-adaptive-spec.mjs" "$output/uke-control.spec" "$contract" "$owner/packaging/adaptive-unit-state.sh"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
(cd "$output";sha256sum -c --strict --quiet "$work/parents.sha256")
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-4.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control217r4 source:55 parent sources byte-exact,identical Adaptive17 Source0 archives,full recipe inverse,explicit optional-plugin dependency coverage and three original outputs.\n'
