#!/usr/bin/bash
# Data-only source assembly; no native compilation or policy-store operation.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 2 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/selinux-integration.json
parent=$(realpath "$1")
output=$(realpath -m "$2")
[[ ! -e $output && ! -L $output ]]
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/overlay"
rpm2cpio "$parent" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
rpm2cpio "$parent" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ -z $(find "$output" -type l -print -quit) ]]
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 62 ]]
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parent.sha256")
for file in packaging/selinux/selinux-kernel-abi.sh packaging/selinux/compose-kernel-abi.awk packaging/check-selinux-integration.sh docs/SELINUX-ORIGINAL.md; do
    install -Dm644 "$owner/$file" "$work/overlay/$file"
done
cp "$contract" "$work/overlay/selinux-integration.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-selinux-integration.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-selinux-spec.mjs" "$output/uke-control.spec" "$contract"
(cd "$output";sha256sum -c --strict --quiet "$work/parent.sha256")
bash "$owner/tests/ssc-unit-state.sh" --data-only "$output/uke-control.spec"
bash "$owner/tests/ssc-refresh.sh" --data-only "$output/uke-control.spec"
bash "$owner/tests/ssc-cache-postun.sh" --data-only "$output/uke-control.spec"
if command -v rpmspec >/dev/null; then
    [[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-15.fc46 ]]
    rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
    printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
else
    printf 'RPM parser deferred to the COPR source worker.\n'
fi
printf 'Control15:62 parent inputs unchanged; entire recipe inverse and inherited SSC state gates pass.\n'
