#!/usr/bin/bash
# COPR source assembly; complete Control26 and its retained tests remain.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1");contract=$owner/packaging/factory-inputs-source.json
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent" "$work/pubkeys" "$work/gnupg" "$work/overlay"
curl -qfsSL --max-time 180 --max-filesize 33554432 "$(jq -er .parent.url "$contract")" -o "$work/parent.src.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=84)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
fingerprint=$(GNUPGHOME="$work/gnupg" gpg --batch --with-colons --show-keys "$work/parent/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print tolower($10);exit}')
[[ $fingerprint == $(jq -er .parent.signer_fingerprint "$contract") ]]
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --import "$work/parent/copr-public-key.asc"
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --checksig --verbose "$work/parent.src.rpm" > "$work/signatures"
[[ $(grep -Eic 'signature.*: OK$' "$work/signatures") == 2 ]]
if grep -Eiq 'NOTFOUND|NOKEY|BAD|NOT OK' "$work/signatures";then exit 1;fi
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/retained.sha256"
[[ $(wc -l < "$work/retained.sha256") == 83 ]]
bash "$owner/packaging/fold-factory-inputs-spec.sh" "$work/parent/uke-control.spec" "$work/new.spec" "$contract"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-control.spec"
for name in apply.sh consumer.patch pre.sha256 post.sha256;do
 install -m644 "$owner/packaging/factory-inputs/$name" "$work/overlay/$name"
done
install -m644 "$owner/src/boot-update/factory-input.hpp" "$work/overlay/"
for path in tests/{factory-input.cpp,factory-input.sh} docs/FACTORY-UPDATE-INPUTS.md;do
 install -Dm644 "$owner/$path" "$work/overlay/$path"
done
printf '%s  %s\n' "$(jq -er .patch_sha256 "$contract")" "$work/overlay/consumer.patch" | sha256sum -c --strict --quiet -
(cd "$work/overlay";find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$work/layer.sha256"
install -m644 "$work/layer.sha256" "$work/overlay/SHA256SUMS"
tar --sort=name --mtime=@1794873600 --owner=0 --group=0 --numeric-owner -cJf "$result/factory-inputs-overlay.tar.xz" -C "$work/overlay" .
install -m644 "$contract" "$result/"
(cd "$result";sha256sum -c --strict --quiet "$work/retained.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-control.spec") == uke-control-2.17.0-27.fc46 ]]
rpmspec --target aarch64 --define 'dist .fc46' -q --qf '%{NAME}\n' "$result/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
