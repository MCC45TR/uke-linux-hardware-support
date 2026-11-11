import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [spec,contract]=process.argv.slice(2),m=JSON.parse(fs.readFileSync(contract));
const before=fs.readFileSync(spec,'utf8'),hash=x=>crypto.createHash('sha256').update(x).digest('hex');
assert.equal(hash(before),m.parent.spec_sha256);
assert.equal(m.result.native_revision,102);
const marker=before.indexOf('%changelog\n');assert(marker>0);
let body=before.slice(0,marker),history=before.slice(marker);
assert.equal(body.split('Release: 100%{?dist}\n').length,2);
body=body.replace('Release: 100%{?dist}\n','Release: 102%{?dist}\n');
const changes=[
 ['Summary: Original Fedora FastRPC readiness integration qualification\n','Summary: Original Fedora FastRPC session integration qualification\n'],
 ['ExclusiveArch: aarch64\n','Source2: uke-fastrpc-session-1.tar.xz\nExclusiveArch: aarch64\n'],
 ['(cd files;sha256sum -c --strict --quiet SHA256SUMS)\n','(cd files;sha256sum -c --strict --quiet SHA256SUMS)\nmkdir session\ntar -xJf %{SOURCE2} -C session --strip-components=1\n(cd session;sha256sum -c --strict --quiet SHA256SUMS)\n'],
 ['%check\n','g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \\\n session/fastrpc-session.cpp -ldl -o session-worker\n' +
  'g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror -fPIC -shared \\\n session/fastrpc-session-mocks.cpp -o session-mocks.so\n' +
  'g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \\\n session/fastrpc-session-test.cpp -o session-fixture\n' +
  'g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \\\n -Isession/reference session/fastrpc-session-abi.cpp -o session-abi\n\n%check\n'],
 ['test ! -e files-fixture-root\n','test ! -e files-fixture-root\n' +
  './session-abi\nbash session/fastrpc-session-test.sh "$PWD/session-fixture" "$PWD/session-worker" "$PWD/libuke-fastrpc-notify.so" "$PWD/session-mocks.so"\n'],
 ['%install\n','printf \'Original Fedora single-session lifecycle and source ABI checks passed.\\n\' >> %{buildroot}%{_datadir}/uke-kernel-migration-check/fastrpc-notify.txt\n\n%install\n'],
 ['%files\n','%files\n%doc session/FASTRPC-SESSION.md session/session-source-receipt.json\n'],
];
for(const [a,b]of changes){assert.equal(body.split(a).length,2,'Ambiguous anchor:'+a);body=body.replace(a,()=>b);}
let inverse=body;
for(const [a,b]of changes.toReversed()){assert.equal(inverse.split(b).length,2,'Inverse anchor');inverse=inverse.replace(b,()=>a);}
inverse=inverse.replace('Release: 102%{?dist}\n','Release: 100%{?dist}\n');
assert.equal(inverse+history,before,'Unrelated native100 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 7.3.3-102\n- Qualify Control session policy with original Fedora FastRPC and kernel ABI.\n- Retain all prior readiness and file API checks with exact parent sources.\n\n';
fs.writeFileSync(spec,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Native102:whole native100 recipe inverse,exact parent sources,one session-policy fixture.');
