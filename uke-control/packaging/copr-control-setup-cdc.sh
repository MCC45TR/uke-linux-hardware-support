#!/usr/bin/bash
# Source-only assembly; native compilation and real prepared-header tests run in COPR.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1");contract=$owner/packaging/setup-cdc-source.json
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent" "$work/pubkeys" "$work/gnupg" "$work/overlay"
curl -qfsSL --max-time 180 --max-filesize 25165824 "$(jq -er .parent.url "$contract")" -o "$work/parent.src.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk -v count_expected="$(jq -er '.parent.source_count+1' "$contract")" '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=count_expected)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
fingerprint=$(GNUPGHOME="$work/gnupg" gpg --batch --with-colons --show-keys "$work/parent/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print tolower($10);exit}')
[[ $fingerprint == $(jq -er .parent.signer_fingerprint "$contract") ]]
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --import "$work/parent/copr-public-key.asc"
verify() {
 rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --checksig --verbose "$1" > "$work/signatures"
 [[ $(grep -Eic 'signature.*: OK$' "$work/signatures") == 2 ]]
 ! grep -Eiq 'NOTFOUND|NOKEY|BAD|NOT OK' "$work/signatures"
}
verify "$work/parent.src.rpm"
curl -qfsSL --max-time 90 --max-filesize 1048576 "$(jq -er .native_proof.url "$contract")" -o "$result/setup-cdc-native-proof.rpm"
printf '%s  %s\n' "$(jq -er .native_proof.sha256 "$contract")" "$result/setup-cdc-native-proof.rpm" | sha256sum -c --strict --quiet -
verify "$result/setup-cdc-native-proof.rpm"
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/inputs.sha256"
[[ $(wc -l < "$work/inputs.sha256") == 74 ]]
bash "$owner/packaging/fold-setup-cdc-spec.sh" "$work/parent/uke-control.spec" "$work/new.spec" "$contract"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-control.spec"
for path in src/setup-cdc.cpp src/setup-cdc.hpp tests/setup-cdc.cpp tests/setup-cdc.sh packaging/apply-setup-cdc.sh \
 packaging/uke-control-setup-cdc.service packaging/uke-control-setup-console.service \
 packaging/uke-control-setup-cdc-finish.service packaging/uke-control-setup-cdc-finish.path \
 packaging/91-uke-control-setup-cdc.rules packaging/81-uke-control-setup-cdc.preset;do
 [[ -f $owner/$path && ! -L $owner/$path ]]
 install -Dm644 "$owner/$path" "$work/overlay/$path"
done
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
tar --sort=name --mtime=@1794787200 --owner=0 --group=0 --numeric-owner -cJf "$result/setup-cdc-overlay.tar.xz" -C "$work/overlay" .
install -m644 "$contract" "$result/"
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-control.spec") == uke-control-2.17.0-23.fc46 ]]
rpmspec --target aarch64 --define 'dist .fc46' -q --qf '%{NAME}\n' "$result/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
printf 'Control23 retains all 74 signed22 inputs and complete recipe inverse; setup is disabled until a fresh image opts in.\n'
