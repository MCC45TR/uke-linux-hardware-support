import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [spec,contract]=process.argv.slice(2),m=JSON.parse(fs.readFileSync(contract));
const before=fs.readFileSync(spec,'utf8'),hash=x=>crypto.createHash('sha256').update(x).digest('hex');
assert.equal(hash(before),m.parent.spec_sha256);
assert.equal(m.result.native_revision,100);
const marker=before.indexOf('%changelog\n');assert(marker>0);
let body=before.slice(0,marker),history=before.slice(marker);
assert.equal(body.split('Release: 99%{?dist}\n').length,2);
body=body.replace('Release: 99%{?dist}\n','Release: 100%{?dist}\n');
const changes=[
 ['License: MIT\n','License: MIT AND BSD-3-Clause\n'],
 ['ExclusiveArch: aarch64\n','Source1: uke-fastrpc-files-1.tar.xz\nExclusiveArch: aarch64\n'],
 ['sha256sum -c --strict --quiet SHA256SUMS\n','sha256sum -c --strict --quiet SHA256SUMS\nmkdir files\ntar -xJf %{SOURCE1} -C files --strip-components=1\n(cd files;sha256sum -c --strict --quiet SHA256SUMS)\n'],
 ['%check\n','g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \\\n -Ifiles/upstream -I/usr/include/fastrpc files/fastrpc-files.cpp -ldl -o files-fixture\n\n%check\n'],
 ['bash fastrpc-notify-test.sh "$PWD/fixture" "$PWD/libuke-fastrpc-notify.so"\n','bash fastrpc-notify-test.sh "$PWD/fixture" "$PWD/libuke-fastrpc-notify.so"\nprintf \'%%s  %%s\\n\' "$(jq -er .fedora.library_elf_sha256 files/fastrpc-files.json)" /usr/lib64/libadsprpc.so.1.0.0 | sha256sum -c --strict --quiet -\nmkdir files-fixture-root\ntimeout 15 ./files-fixture "$PWD/files-fixture-root"\ntest ! -e files-fixture-root\n'],
 ['%files\n','%files\n%license files/LICENSE.fastrpc\n%doc files/FASTRPC-FILES.md files/files-source-receipt.json\n'],
 ['%install\n','printf \'Original Fedora file APIs passed with all ioctl syscalls denied.\\n\' >> %{buildroot}%{_datadir}/uke-kernel-migration-check/fastrpc-notify.txt\n\n%install\n'],
];
for(const [a,b]of changes){assert.equal(body.split(a).length,2,'Ambiguous anchor:'+a);body=body.replace(a,()=>b);}
let inverse=body;
for(const [a,b]of changes.toReversed()){assert.equal(inverse.split(b).length,2,'Inverse anchor');inverse=inverse.replace(b,()=>a);}
inverse=inverse.replace('Release: 100%{?dist}\n','Release: 99%{?dist}\n');
assert.equal(inverse+history,before,'Unrelated native99 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 7.3.3-100\n- Qualify original Fedora file APIs with device ioctl syscalls refused.\n- Retain all prior listener registration checks and exact source inputs.\n\n';
fs.writeFileSync(spec,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Native100:whole native99 recipe inverse,original source preserved,one additive file-API fixture.');
