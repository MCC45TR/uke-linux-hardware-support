#!/usr/bin/bash
# Source worker only. Reuse the actual217 source and qualified94 wait delta.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd);workspace=$(dirname "$owner")
contract=$owner/packaging/charge-wait-source.json
delivery=$owner/packaging/charge-wait-delivery.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$delivery" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/parent.rpm") == uke-control-2.17.0-1 ]]
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$work/overlay"
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
curl -qfsSL --max-time 90 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/11304610-uke-kernel-migration-check/uke-kernel-migration-check-7.3.3-94.fc46.aarch64.rpm -o "$work/proof.rpm"
printf '%s  %s\n' a03b2cbde163a83e18a52454accaa8910d733e23cf4487ee581b15200b981b50 "$work/proof.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/proof.rpm" | cpio -i --to-stdout --quiet '*control-wait-source-qualification.json' > "$work/overlay/native94.json"
jq -e '.source_commit=="088a1ed3a950ee4c04b307fc0405ec5a1ebb62f8" and .parent.build==11303736 and .parent.version=="2.17.0-1" and .source_unit.after_sha256=="c5e1600477f16593ff6b83c68857f076aecae6fb8a04bd72925c1d0ebcf1c678"' "$work/overlay/native94.json" >/dev/null
for name in charge-wait.patch charge-wait-source.json apply-charge-wait.sh check-charge-wait-native.sh;do
 install -m644 "$owner/packaging/$name" "$work/overlay/$name"
done
install -m644 "$workspace/uke-platform-runtime/tests/charge-wait.cpp" "$work/overlay/charge-wait.cpp"
install -m644 "$owner/docs/CHARGE-WAIT.md" "$work/overlay/CHARGE-WAIT.md"
commit=$(git -C "$workspace" rev-parse HEAD);tree=$(git -C "$workspace" rev-parse HEAD^{tree})
jq --arg commit "$commit" --arg tree "$tree" '.+{source_commit:$commit,source_tree:$tree}' "$delivery" > "$work/overlay/delivery-source.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-charge-wait.tar.xz" -C "$work/overlay" .
bash "$owner/packaging/fold-charge-wait-spec.sh" "$output/uke-control.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-2.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control217r2 source: original51 source files retained,one qualified overlay and three existing RPM names; no new runtime service or dependency.\n'
