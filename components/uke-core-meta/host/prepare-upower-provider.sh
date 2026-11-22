#!/usr/bin/bash
# Data-only source preparation; never compiles, installs or changes providers.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 2 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
manifest=$owner/manifests/original-upower-provider.json
parent=$(realpath "$1")
output=$(realpath -m "$2")
[[ ! -e $output ]]
printf '%s  %s\n' "$(jq -er .parent_srpm_sha256 "$manifest")" "$parent" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$parent") == uke-core-meta-1.0.7-21 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
rpm2cpio "$parent" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | sort > "$work/paths"
printf '%s\n' ./uke-core-meta-1.0.7.tar.xz ./uke-core-meta.spec | cmp - "$work/paths"
mkdir "$work/source"
(cd "$work/source";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
for entry in "$work/source/uke-core-meta.spec" "$work/source/uke-core-meta-1.0.7.tar.xz";do
 [[ -f $entry && ! -L $entry ]]
done
printf '%s  %s\n' "$(jq -er .parent_spec_sha256 "$manifest")" "$work/source/uke-core-meta.spec" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .source_archive_sha256 "$manifest")" "$work/source/uke-core-meta-1.0.7.tar.xz" | sha256sum -c --strict --quiet -
node - "$work/source/uke-core-meta.spec" "$work/candidate.spec" "$manifest" <<'NODE'
const fs=require('fs'),[input,output,manifest]=process.argv.slice(2),m=JSON.parse(fs.readFileSync(manifest)),s=fs.readFileSync(input,'utf8');
const changes=[['Release: 21%{?dist}',`Release: ${m.release}%{?dist}`],[m.requires_before,m.requires_after],['%changelog\n','%changelog\n* Wed Nov 11 2026 Uke contributors - 1.0.7-22\n- Accept the standard UPower provider; preserve payload and other selection.\n\n']];
let next=s;for(const [before,after]of changes){if(next.split(before).length!==2)throw Error('Ambiguous source boundary');next=next.replace(before,after);}
let inverse=next;for(const [before,after]of changes.toReversed()){if(inverse.split(after).length!==2)throw Error('Ambiguous inverse');inverse=inverse.replace(after,before);}
if(inverse!==s)throw Error('Unreviewed spec delta');
fs.writeFileSync(output,next);
NODE
if command -v rpmspec >/dev/null;then
 [[ $(rpmspec --target aarch64 -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$work/candidate.spec") == uke-core-meta-1.0.7-22.fc46.aarch64 ]]
else
 printf 'RPM spec parsing deferred to COPR; rpmspec is unavailable locally.\n'
fi
# The existing %check validates every payload datum during the COPR build.
mkdir "$output"
install -m644 "$work/candidate.spec" "$output/uke-core-meta.spec"
install -m644 "$work/source/uke-core-meta-1.0.7.tar.xz" "$output/uke-core-meta-1.0.7.tar.xz"
cmp "$work/source/uke-core-meta-1.0.7.tar.xz" "$output/uke-core-meta-1.0.7.tar.xz"
printf 'Core22 source prepared: unchanged archive and reversible dependency/release/changelog delta; no provider retirement, install or runtime parity claim.\n'
