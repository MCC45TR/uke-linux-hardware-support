#!/usr/bin/bash
# COPR source worker only. Reuse accepted Control12 and exact pogo source offers.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/pogo-services.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output && -z $(git -C "$workspace" status --porcelain) ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/overlay" "$work/overlay/upstreams"
extract_srpm(){
 local key=$1 destination=$2
 mkdir "$destination"
 curl -qfsSL --retry 2 --max-time 120 "$(jq -er "$key.url" "$contract")" -o "$work/input.rpm"
 printf '%s  %s\n' "$(jq -er "$key.srpm_sha256" "$contract")" "$work/input.rpm" | sha256sum -c --strict --quiet -
 rpm2cpio "$work/input.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
 rpm2cpio "$work/input.rpm" | (cd "$destination";cpio -idm --quiet --no-absolute-filenames)
}
extract_srpm .parent "$work/parent"
[[ $(find "$work/parent" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 61 ]]
cp -a "$work/parent/." "$output/"
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parent.sha256")
for component in auth desktop;do
 extract_srpm ".inputs.$component" "$work/$component-inputs"
 case "$component" in auth) archive=uke-pogo-auth-0.1.tar.gz;;desktop) archive=uke-pogo-desktop-0.1.tar.gz;;esac
 tar -tf "$work/$component-inputs/$archive" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
 mkdir "$work/overlay/$component"
 tar -xf "$work/$component-inputs/$archive" --strip-components=1 -C "$work/overlay/$component"
 [[ -z $(find "$work/overlay/$component" -type l -print -quit) ]]
done
for library in minkipc quic-teec qcbor;do cp "$work/auth-inputs/$library.tar.gz" "$work/overlay/upstreams/";done
printf '2da103029796465773fa77746cf10799ff1fb43c62ba8dbb1efc51b62e3dd36f  %s/minkipc.tar.gz\n887ace80e98c0dc67163a44fdd280c9a9cf0e46a9671ebf33643def261fc405a  %s/quic-teec.tar.gz\n93fa4b2e56a2fe5af32be5e3c05a95d4563daea4b127023d6f77beb2274bf6ae  %s/qcbor.tar.gz\n' \
 "$work/overlay/upstreams" "$work/overlay/upstreams" "$work/overlay/upstreams" | sha256sum -c --strict --quiet -
node "$owner/packaging/patch-pogo-sources.mjs" "$work/overlay" "$owner" "$contract"
for file in src/pogo-runtime.hpp tests/pogo-runtime.cpp tests/pogo-runtime.sh tests/pogo-ownership.sh tests/pogo-fedora-credentials.c packaging/build-pogo-auth.sh packaging/pogo-ownership.sh packaging/check-pogo-integration.sh docs/POGO-INTEGRATION.md;do
 install -Dm644 "$owner/$file" "$work/overlay/$file"
done
jq --arg commit "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" \
 '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/overlay/pogo-services.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-pogo-integration.tar.xz" -C "$work/overlay" .
node "$owner/packaging/fold-pogo-spec.mjs" "$output/uke-control.spec" "$contract" "$owner"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
(cd "$output";sha256sum -c --strict --quiet "$work/parent.sha256")
bash "$owner/tests/pogo-ownership.sh" "$owner"
bash "$owner/tests/ssc-unit-state.sh" --data-only "$output/uke-control.spec"
bash "$owner/tests/ssc-refresh.sh" --data-only "$output/uke-control.spec"
bash "$owner/tests/ssc-cache-postun.sh" --data-only "$output/uke-control.spec"
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-13.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control13:61 actual parent inputs exact,whole recipe/source inverses,pogo ownership and inherited SSC state cases;three existing output names.\n'
