#!/usr/bin/bash
# Data-only Core23 to Core24 dependency transition; payload stays unchanged.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 2 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
manifest=$owner/manifests/original-selinux-provider.json
parent=$(realpath "$1")
output=$(realpath -m "$2")
[[ ! -e $output && ! -L $output ]]
printf '%s  %s\n' "$(jq -er .parent_srpm_sha256 "$manifest")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
rpm2cpio "$parent" > "$work/input.cpio"
cpio -t --quiet < "$work/input.cpio" | sort > "$work/names"
printf '%s\n' ./uke-core-meta-1.0.7.tar.xz ./uke-core-meta.spec | cmp - "$work/names"
mkdir "$work/input"
(cd "$work/input";cpio -idm --quiet --no-absolute-filenames < "$work/input.cpio")
[[ -z $(find "$work/input" -type l -print -quit) ]]
printf '%s  %s\n' "$(jq -er .parent_spec_sha256 "$manifest")" "$work/input/uke-core-meta.spec" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .source_archive_sha256 "$manifest")" "$work/input/uke-core-meta-1.0.7.tar.xz" | sha256sum -c --strict --quiet -
node --input-type=module - "$work/input/uke-core-meta.spec" "$work/candidate.spec" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';
const [input,output]=process.argv.slice(2), before=fs.readFileSync(input,'utf8');
const changes=[['Release: 23%{?dist}','Release: 24%{?dist}'],
 ['Requires: uke-control(fedora-returns-v1) >= 1\n','Requires: uke-control(fedora-returns-v1) >= 1\nRequires: uke-control(selinux-kernel-abi-v1) >= 1\nRequires: selinux-policy >= 45.24-1\nRequires: selinux-policy-targeted >= 45.24-1\n'],
 ['%changelog\n','%changelog\n* Thu Nov 12 2026 Uke contributors - 1.0.7-24\n- Require original Fedora SELinux with the existing Control ABI integration.\n- Preserve every Core23 payload and unrelated dependency.\n\n']];
let after=before;for(const [old,value]of changes){assert.equal(after.split(old).length,2);after=after.replace(old,()=>value);}
let inverse=after;for(const [old,value]of changes.toReversed()){assert.equal(inverse.split(value).length,2);inverse=inverse.replace(value,()=>old);}assert.equal(inverse,before);
fs.writeFileSync(output,after);
JS
mkdir "$output"
install -m644 "$work/candidate.spec" "$output/uke-core-meta.spec"
install -m644 "$work/input/uke-core-meta-1.0.7.tar.xz" "$output/uke-core-meta-1.0.7.tar.xz"
if command -v rpmspec >/dev/null;then
 [[ $(rpmspec --target aarch64 -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$output/uke-core-meta.spec") == uke-core-meta-1.0.7-24.fc46.aarch64 ]]
fi
printf 'Core24: entire Core23 recipe inverse and unchanged payload pass; DNF/device gates remain separate.\n'
