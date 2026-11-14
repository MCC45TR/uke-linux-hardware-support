#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR source assembly only; the tablet never runs native build or policy tools.
set -Eeuo pipefail
export LC_ALL=C
umask 077
[[ $# == 3 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $EUID != 0 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
contract=$owner/packaging/motion-integration.json
parent=$(realpath -e -- "$1")
policy=$(realpath -e -- "$2")
output=$(realpath -m -- "$3")
[[ ! -e $output && ! -L $output ]]
[[ $(stat -c %s "$parent") -le 16777216 && $(stat -c %s "$policy") -le 8388608 ]]
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .policy_fixture.sha256 "$contract")" "$policy" | sha256sum -c --strict --quiet -
(cd "$workspace";node uke-control/packaging/motion-assets.mjs --check)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
rpm2cpio "$parent" | cpio -t --quiet > "$work/members"
awk '
 {name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}
 END{if(bad||count!=65)exit 1}' "$work/members"
mkdir "$output" "$work/overlay" "$work/rpmdb" "$work/gnupg"
rpm2cpio "$parent" | (cd "$output";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
[[ -z $(find "$output" -mindepth 1 ! -type f -print -quit) ]]
node - "$output" "$contract" <<'JS'
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict'),path=require('node:path');
const [root,manifest]=process.argv.slice(2),c=JSON.parse(fs.readFileSync(manifest));
assert.equal(c.parent.inputs.length,64);
assert.deepEqual(fs.readdirSync(root).sort(),[...c.parent.inputs.map(x=>x.path),'uke-control.spec'].sort());
for(const input of c.parent.inputs){assert(/^[A-Za-z0-9_.+-]+$/.test(input.path));const data=fs.readFileSync(path.join(root,input.path));assert.equal(data.length,input.size);assert.equal(crypto.createHash('sha256').update(data).digest('hex'),input.sha256);}
assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(root,'uke-control.spec'))).digest('hex'),c.parent.spec_sha256);
console.log('Signed16 source inventory: all64 input names, lengths and hashes match.');
JS
fingerprint=$(GNUPGHOME="$work/gnupg" gpg --batch --with-colons --show-keys "$output/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print $10;exit}')
[[ ${fingerprint,,} == "$(jq -er .parent.signer_fingerprint "$contract")" ]]
rpm --dbpath "$work/rpmdb" --initdb
rpmkeys --dbpath "$work/rpmdb" --import "$output/copr-public-key.asc"
rpmkeys --dbpath "$work/rpmdb" --checksig --verbose "$parent" > "$work/signatures"
[[ $(grep -Eic 'signature.*: OK$' "$work/signatures") == 2 ]]
! grep -Eiq 'NOTFOUND|NOKEY|BAD|NOT OK' "$work/signatures"
printf 'Parent16 has two trusted signatures and valid digests.\n'
(cd "$output";find . -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum > "$work/parent.sha256")
while IFS= read -r asset;do
 [[ $asset == uke-control/* && $asset != *..* ]]
 install -Dm644 "$workspace/$asset" "$work/overlay/$asset"
done < <(jq -r '.source_assets[].path' "$contract")
install -Dm644 "$contract" "$work/overlay/uke-control/packaging/motion-integration.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(jq -er .source_epoch "$contract")
[[ $epoch =~ ^[0-9]{10}$ ]]
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-motion.tar.xz" -C "$work/overlay" .
install -m644 "$policy" "$output/original-fedora-motion-policy.rpm"
node "$owner/packaging/fold-motion-spec.mjs" "$output/uke-control.spec" "$contract" "$owner"
(cd "$output";sha256sum -c --strict --quiet "$work/parent.sha256")
for fixture in ssc-unit-state ssc-refresh ssc-cache-postun motion-lifecycle;do
 bash "$owner/tests/$fixture.sh" --data-only "$output/uke-control.spec"
done
[[ $(rpmspec --target aarch64 --define 'dist .fc46' --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-19.fc46 ]]
rpmspec --target aarch64 --define 'dist .fc46' --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-control.spec | wc -l) == 66 ]]
printf 'Control19: all64 parent inputs preserved, full recipe inverse passed,66 sources and exactly three existing RPM names.\n'
