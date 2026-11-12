#!/usr/bin/bash
# COPR source worker only. Preserve every Control6 source; add one provider layer.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/fastrpc-provider.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output && -z $(git -C "$workspace" status --porcelain) ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/overlay"
curl -qfsSL --retry 2 --max-time 120 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 60 ]]
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parent.sha256")
for file in src/ssc-cache.cpp tests/ssc-cache-migration.cpp tests/systemd-verify-runtime.cpp tests/systemd-verify-runtime-probe.cpp packaging/stage-fastrpc-root.sh packaging/check-fastrpc-provider.sh docs/FASTRPC-PROVIDER.md;do
 install -Dm644 "$owner/$file" "$work/overlay/$file"
done
for file in "$owner"/src/fastrpc/*;do install -Dm644 "$file" "$work/overlay/src/fastrpc/$(basename "$file")";done
jq --arg commit "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" \
 '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/overlay/fastrpc-provider.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-fastrpc-provider.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-fastrpc-provider-spec.mjs" "$output/uke-control.spec" "$contract"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
(cd "$output";sha256sum -c --strict --quiet "$work/parent.sha256")
bash "$owner/tests/ssc-unit-state.sh" --data-only "$output/uke-control.spec"
bash "$owner/tests/ssc-refresh.sh" --data-only "$output/uke-control.spec"
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-9.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control9:60 actual Control6 sources unchanged;single provider layer,whole recipe inverse,5-unit state and16 refresh cases;three existing output names only.\n'
