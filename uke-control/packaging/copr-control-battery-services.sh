#!/usr/bin/bash
# COPR source worker only; retain actual217r2 and original battery7 payloads.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd);workspace=$(dirname "$owner")
contract=$owner/packaging/battery-services.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for parent in control battery;do
 curl -qfsSL --retry 2 --max-time 180 "$(jq -er ".$parent.url" "$contract")" -o "$work/$parent.rpm"
 printf '%s  %s\n' "$(jq -er ".$parent.srpm_sha256" "$contract")" "$work/$parent.rpm" | sha256sum -c --strict --quiet -
 [[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/$parent.rpm") == "$(jq -er ".$parent.nvr" "$contract")" ]]
 rpm2cpio "$work/$parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
done
mkdir "$output" "$work/battery" "$work/overlay"
rpm2cpio "$work/control.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 52 ]]
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parents.sha256")
rpm2cpio "$work/battery.rpm" | (cd "$work/battery";cpio -idm --quiet --no-absolute-filenames)
printf '%s  %s\n' "$(jq -er .battery.source_archive_sha256 "$contract")" "$work/battery/uke-battery-observer-1.4.1.tar.xz" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .battery.sysusers_sha256 "$contract")" "$work/battery/uke-battery.conf" | sha256sum -c --strict --quiet -
install -m644 "$work/battery/uke-battery-observer-1.4.1.tar.xz" "$output/retained-battery-services-1.4.1-7.tar.xz"
install -m644 "$work/battery/uke-battery.conf" "$output/uke-battery.conf"
install -m644 "$owner/packaging/battery-unit-state.sh" "$work/overlay/"
install -m644 "$owner/tests/battery-unit-state.sh" "$work/overlay/battery-unit-state-test.sh"
install -m644 "$owner/docs/BATTERY-SERVICES.md" "$work/overlay/"
jq --arg commit "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/overlay/battery-services.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-battery-services.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-battery-spec.mjs" "$output/uke-control.spec" "$owner/packaging/battery-unit-state.sh"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
(cd "$output";sha256sum -c --strict --quiet "$work/parents.sha256")
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-3.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control217r3 source:52 parent source files byte-exact;unchanged battery/offline source,three original output names,no replacement of charge helper.\n'
