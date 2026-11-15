#!/usr/bin/bash
# COPR source worker; compilation only in the native RPM build phase.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
contract=$owner/packaging/usb-api-output-source.json
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/sources" "$work/pubkeys" "$work/gnupg"
curl -qfsSL --max-time 180 --max-filesize 25165824 "$(jq -er .parent.url "$contract")" -o "$work/parent.src.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=69)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/sources";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
fingerprint=$(GNUPGHOME="$work/gnupg" gpg --batch --with-colons --show-keys "$work/sources/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print tolower($10);exit}')
[[ $fingerprint == "$(jq -er .parent.signer_fingerprint "$contract")" ]]
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --import "$work/sources/copr-public-key.asc"
curl -qfsSL --max-time 90 --max-filesize 4194304 "$(jq -er .parent.runtime_url "$contract")" -o "$work/runtime.rpm"
printf '%s  %s\n' "$(jq -er .parent.runtime_sha256 "$contract")" "$work/runtime.rpm" | sha256sum -c --strict --quiet -
for file in "$work/parent.src.rpm" "$work/runtime.rpm";do
 rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --checksig --verbose "$file" > "$work/signatures"
 [[ $(grep -Eic 'signature.*: OK$' "$work/signatures") == 2 ]]
 ! grep -Eiq 'NOTFOUND|NOKEY|BAD|NOT OK' "$work/signatures"
done
(cd "$work/sources";find . -maxdepth 1 -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/parent-inputs.sha256"
printf '%s  %s\n' "$(jq -er .patch_sha256 "$contract")" "$owner/packaging/usb-api-output.patch" \
 "$(jq -er .fixture_sha256 "$contract")" "$owner/tests/rpm-api-output.sh" | sha256sum -c --strict --quiet -
bash "$owner/packaging/fold-usb-api-output-spec.sh" "$work/sources/uke-control.spec" "$work/next.spec" "$contract"
install -m644 "$work/next.spec" "$work/sources/uke-control.spec"
install -m644 "$owner/packaging/usb-api-output.patch" "$contract" "$owner/tests/rpm-api-output.sh" "$work/sources/"
install -m644 "$work/runtime.rpm" "$work/sources/control20-parent-runtime.rpm"
(cd "$work/sources";sha256sum -c --strict --quiet "$work/parent-inputs.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/sources/uke-control.spec") == uke-control-2.17.0-21.fc46 ]]
rpmspec --target aarch64 --define 'dist .fc46' -q --qf '%{NAME}\n' "$work/sources/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
cp -a "$work/sources/." "$result/"
printf 'Control21: trusted signed20 source/runtime, every68 parent input and complete recipe inverse retained;72 sources ready.\n'
