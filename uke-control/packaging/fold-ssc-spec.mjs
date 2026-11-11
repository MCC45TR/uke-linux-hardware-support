import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [spec,contract,statePath,blocksPath,nativeSpec]=process.argv.slice(2);
const m=JSON.parse(fs.readFileSync(contract)),b=JSON.parse(fs.readFileSync(blocksPath));
const before=fs.readFileSync(spec,'utf8'),native=fs.readFileSync(nativeSpec,'utf8');
const sha=x=>crypto.createHash('sha256').update(x).digest('hex');
assert.equal(sha(before),m.control.spec_sha256);
assert.equal(sha(native),m.native.spec_sha256);
assert.equal(m.result.version,'2.17.0-5');
const split=before.indexOf('%changelog\n');assert(split>0);
let body=before.slice(0,split),history=before.slice(split);
assert.equal(body.split('Release: 4%{?dist}\n').length,4);
assert.equal((body.match(/2[.]17[.]0-4/g)||[]).length,12);
body=body.replaceAll('Release: 4%{?dist}\n','Release: 5%{?dist}\n').replaceAll('2.17.0-4','2.17.0-5');
const nativeBuild=native.split('%build\n')[1].split('%check\n')[0];
const nativeCheck=native.split('%check\n')[1].split('%install\n')[0];
const marker="printf 'Original Fedora package/header/ELF";
assert.equal(nativeCheck.split(marker).length,2);
const qualified=(section)=>'(\ncd %{_builddir}/uke-control-ssc-overlay/qualified\n'+section+')\n';
const state=fs.readFileSync(statePath,'utf8').replaceAll('%','%%');
const migration=(phase)=>'(\n# UKE_CONTROL_SSC_'+phase.toUpperCase()+'_BEGIN\nphase='+phase+'\n'+state+'# UKE_CONTROL_SSC_'+phase.toUpperCase()+'_END\n) || exit $?\n';
const changes=[
 ['\n%description\n','\n'+b.header+'\n%description\n'],
 ['%build\n',b.prep+'\n%build\n'+b.build+qualified(nativeBuild)],
 ['%install\n','%install\n'+b.install],
 ['%check\n','%check\n'+b.check+qualified(nativeCheck.split(marker)[0])],
 ['%pre -n uke-control\n','%pre -n uke-control\n'+migration('save')],
 ['%posttrans -n uke-control\n','%posttrans -n uke-control\n'+migration('restore')],
 ['%post -n uke-control\n','%post -n uke-control\n%systemd_post uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service\n'],
 ['%preun -n uke-control\n','%preun -n uke-control\n%systemd_preun uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service\n'],
 ['%files\n',b.trigger+'%files\n'+b.files],
];
for(const [a,c]of changes){assert.equal(body.split(a).length,2,'Ambiguous anchor:'+a);body=body.replace(a,()=>c);}
let inverse=body;
for(const [a,c]of changes.toReversed()){assert.equal(inverse.split(c).length,2,'Inverse anchor');inverse=inverse.replace(c,()=>a);}
inverse=inverse.replaceAll('Release: 5%{?dist}\n','Release: 4%{?dist}\n').replaceAll('2.17.0-5','2.17.0-4');
assert.equal(inverse+history,before,'Unrelated Control4 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 2.17.0-5\n- Fold SSC service ownership into Control while preserving the current provider.\n- Qualify the original Fedora library session path without selecting it yet.\n\n';
fs.writeFileSync(spec,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Control5:whole Control4 inverse,unchanged SSC32 provider and complete native102 fixtures.');
