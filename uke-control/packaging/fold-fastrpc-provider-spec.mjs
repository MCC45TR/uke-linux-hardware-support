import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [path,contract]=process.argv.slice(2),m=JSON.parse(fs.readFileSync(contract));
const before=fs.readFileSync(path,'utf8');
assert.equal(m.result.version,'2.17.0-7');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'),m.parent.spec_sha256);
const split=before.indexOf('%changelog\n');assert(split>0);
let body=before.slice(0,split);const history=before.slice(split);
assert.equal(body.split('Release: 6%{?dist}\n').length,4);
assert.equal((body.match(/2[.]17[.]0-6/g)||[]).length,12);
body=body.replaceAll('Release: 6%{?dist}\n','Release: 7%{?dist}\n').replaceAll('2.17.0-6','2.17.0-7');
const prep=`
fastrpc_overlay=%{_builddir}/uke-control-fastrpc-provider
mkdir "$fastrpc_overlay"
tar -xJf %{SOURCE60} -C "$fastrpc_overlay"
(cd "$fastrpc_overlay";sha256sum -c --strict --quiet SHA256SUMS)
`;
const build=`
(
cd %{_builddir}/uke-control-fastrpc-provider
%set_build_flags
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror src/ssc-cache.cpp -o uke-ssc-cache
)
`;
const install=`
fastrpc_overlay=%{_builddir}/uke-control-fastrpc-provider
for unit_file in "$fastrpc_overlay"/src/fastrpc/*;do
 install -Dm644 "$unit_file" "%{buildroot}/usr/lib/systemd/system/$(basename "$unit_file")"
done
bash "$fastrpc_overlay/packaging/stage-fastrpc-root.sh" %{buildroot}/usr/share/uke-control/fastrpc-root
`;
const check=`
bash %{_builddir}/uke-control-fastrpc-provider/packaging/check-fastrpc-provider.sh %{_builddir}/uke-control-fastrpc-provider %{_builddir}/uke-control-ssc-overlay %{buildroot}
`;
const oldUnits='for name in cache root sensors;do\n install -Dm644 "$ssc/inputs/uke-ssc-$name.service" %{buildroot}/usr/lib/systemd/system/uke-ssc-$name.service\ndone\n';
const changes=[
 ['Source59: uke-fastrpc-session-1.tar.xz\n','Source59: uke-fastrpc-session-1.tar.xz\nSource60: uke-control-fastrpc-provider.tar.xz\n'],
 ['\n%prep\n','\n%prep\n'+prep],
 ['\n%build\n','\n%build\n'+build],
 ['\n%install\n','\n%install\n'+install],
 ['install -Dm755 "$ssc/legacy/uke-hexagonrpcd" %{buildroot}/usr/libexec/uke-hexagonrpcd\n',''],
 ['install -Dm755 "$ssc/legacy/uke-ssc-cache" %{buildroot}/usr/libexec/uke-ssc-cache\n','install -Dm755 "$fastrpc_overlay/uke-ssc-cache" %{buildroot}/usr/libexec/uke-ssc-cache\n'],
 [oldUnits,''],
 ['\n%check\n','\n%check\n'+check],
 ['%doc SSC-SERVICES.md\n/usr/libexec/uke-hexagonrpcd\n','%doc %{_builddir}/uke-control-fastrpc-provider/docs/FASTRPC-PROVIDER.md\n%dir /usr/share/uke-control\n/usr/share/uke-control/fastrpc-root\n/usr/lib/systemd/system/run-uke*.mount\n'],
];
// Keep markers for deleted blocks so complete reversal remains unambiguous.
const applied=[];
for(const [old,replacement]of changes){assert.equal(body.split(old).length,2,'Ambiguous provider delta');const next=replacement||`# UKE_FASTRPC_RETIRED_BLOCK_${applied.length}\n`;body=body.replace(old,()=>next);applied.push([old,next]);}
let inverse=body;
for(const [old,next]of applied.toReversed()){assert.equal(inverse.split(next).length,2,'Ambiguous provider inverse');inverse=inverse.replace(next,()=>old);}
inverse=inverse.replaceAll('Release: 7%{?dist}\n','Release: 6%{?dist}\n').replaceAll('2.17.0-7','2.17.0-6');
assert.equal(inverse+history,before,'Unrelated Control6 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 2.17.0-7\n- Use original Fedora FastRPC through the existing Control session integration.\n- Preserve the accepted sensor registry when moving to private noswap RAM.\n- Retain source/native regression gates and remove the legacy protocol runtime.\n\n';
fs.writeFileSync(path,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Control7:whole Control6 recipe inverse passed;only declared provider/cache/namespace/packaging deltas.');
