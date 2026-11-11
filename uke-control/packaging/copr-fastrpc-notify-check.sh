#!/usr/bin/bash
# COPR source generation only. Does not compile or install on the tablet.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 2 && $2 == 99 ]]
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(cd "$owner/.." && pwd)
contract=$owner/packaging/fastrpc-notify.json
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
bundle=$work/uke-fastrpc-notify-check-1
mkdir "$bundle" "$output"
install -m644 "$owner/src/fastrpc-notify.cpp" "$bundle/fastrpc-notify.cpp"
install -m644 "$owner/tests/fastrpc-notify.cpp" "$bundle/fastrpc-notify-test.cpp"
install -m644 "$owner/tests/fastrpc-notify.sh" "$bundle/fastrpc-notify-test.sh"
install -m644 "$owner/docs/FASTRPC-NOTIFY.md" "$bundle/FASTRPC-NOTIFY.md"
install -m644 "$owner/LICENSE" "$bundle/LICENSE"
install -m644 "$contract" "$bundle/fastrpc-notify.json"
commit=$(git -C "$owner" rev-parse HEAD)
tree=$(git -C "$owner" rev-parse HEAD^{tree})
jq -n --arg commit "$commit" --arg tree "$tree" '{source_commit:$commit,source_tree:$tree,scope:"Original Fedora listener,synthetic RPC only;no provider transition or physical acceptance",native_revision:99,new_runtime_packages:0}' > "$bundle/source-receipt.json"
(cd "$bundle";find . -type f -print0 | sort -z | xargs -0 sha256sum > "$work/seal")
install -m644 "$work/seal" "$bundle/SHA256SUMS"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-fastrpc-notify-check-1.tar.xz" -C "$work" uke-fastrpc-notify-check-1
sed 's/@RELEASE@/99/g' "$owner/packaging/fastrpc-notify-check.spec.in" > "$output/uke-kernel-migration-check.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-kernel-migration-check.spec") == uke-kernel-migration-check-7.3.3-99.fc46 ]]
printf 'FastRPC readiness99:original Fedora1.0.8 package/header/ELF,local-scope forwarding and synthetic RPC fixture;no runtime delivery.\n'
