#!/usr/bin/bash
# COPR source worker only; preserve native100 and the original Fedora SDK.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 2 && $2 == 101 ]]
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(cd "$owner/.." && pwd)
contract=$owner/packaging/fastrpc-session.json
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/upstream" "$work/bundle"
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f | wc -l) == 3 ]]
jq -r '.parent.archives[]|.sha256+"  "+.filename' "$contract" | (cd "$output";sha256sum -c --strict --quiet -)
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .sdk.url "$contract")" -o "$work/sdk.tar.gz"
printf '%s  %s\n' "$(jq -er .sdk.archive_sha256 "$contract")" "$work/sdk.tar.gz" | sha256sum -c --strict --quiet -
tar -xzf "$work/sdk.tar.gz" -C "$work/upstream"
sdk=$work/upstream/fastrpc-1.0.8/inc/fastrpc_ioctl.h
printf '%s  %s\n' "$(jq -er .sdk.header_sha256 "$contract")" "$sdk" | sha256sum -c --strict --quiet -
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .kernel.url "$contract")" -o "$work/kernel.h"
printf '%s  %s\n' "$(jq -er .kernel.header_sha256 "$contract")" "$work/kernel.h" | sha256sum -c --strict --quiet -
bundle=$work/bundle/uke-fastrpc-session-1
mkdir -p "$bundle/reference/sdk" "$bundle/reference/kernel"
install -m644 "$sdk" "$bundle/reference/sdk/fastrpc_ioctl.h"
install -m644 "$work/kernel.h" "$bundle/reference/kernel/fastrpc.h"
install -m644 "$owner/src/fastrpc-session.cpp" "$bundle/fastrpc-session.cpp"
install -m644 "$owner/tests/fastrpc-session.cpp" "$bundle/fastrpc-session-test.cpp"
for file in fastrpc-session-mocks.cpp fastrpc-session-abi.cpp fastrpc-session-test.sh; do
 install -m644 "$owner/tests/$file" "$bundle/$file"
done
install -m644 "$owner/docs/FASTRPC-SESSION.md" "$bundle/FASTRPC-SESSION.md"
install -m644 "$contract" "$bundle/fastrpc-session.json"
commit=$(git -C "$owner" rev-parse HEAD)
tree=$(git -C "$owner" rev-parse HEAD^{tree})
jq -n --arg commit "$commit" --arg tree "$tree" '{source_commit:$commit,source_tree:$tree,native_revision:101,scope:"Original Fedora single-session launcher and kernel source ABI;all ioctl syscalls denied in session fixture",new_runtime_packages:0}' > "$bundle/session-source-receipt.json"
(cd "$bundle";find . -type f -print0 | sort -z | xargs -0 sha256sum > "$work/seal")
install -m644 "$work/seal" "$bundle/SHA256SUMS"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-fastrpc-session-1.tar.xz" -C "$work/bundle" uke-fastrpc-session-1
node "$owner/packaging/fold-fastrpc-session-spec.mjs" "$output/uke-kernel-migration-check.spec" "$contract"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-kernel-migration-check.spec") == uke-kernel-migration-check-7.3.3-101.fc46 ]]
printf 'Native101:exact native100 plus original-library session fixture and source ABI comparison;no Fedora rebuild/runtime delivery.\n'
