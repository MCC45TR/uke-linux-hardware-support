import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [spec,contract,statePath,blocksPath,nativeSpec]=process.argv.slice(2);
const m=JSON.parse(fs.readFileSync(contract)),b=JSON.parse(fs.readFileSync(blocksPath));
const before=fs.readFileSync(spec,'utf8'),native=fs.readFileSync(nativeSpec,'utf8');
const sha=x=>crypto.createHash('sha256').update(x).digest('hex');
assert.equal(sha(before),m.control.spec_sha256);
assert.equal(sha(native),m.native.spec_sha256);
assert.equal(m.result.version,'2.17.0-6');
const split=before.indexOf('%changelog\n');assert(split>0);
let body=before.slice(0,split),history=before.slice(split);
assert.equal(body.split('Release: 5%{?dist}\n').length,4);
assert.equal((body.match(/2[.]17[.]0-5/g)||[]).length,12);
body=body.replaceAll('Release: 5%{?dist}\n','Release: 6%{?dist}\n').replaceAll('2.17.0-5','2.17.0-6');
const state=fs.readFileSync(statePath,'utf8').replaceAll('%','%%');
const migration=(phase)=>'# UKE_CONTROL_SSC_'+phase+'_BEGIN\nphase='+phase.toLowerCase()+'\n'+state+'# UKE_CONTROL_SSC_'+phase+'_END\n';
const oldMigration=phase=>{
 const start=body.indexOf('# UKE_CONTROL_SSC_'+phase+'_BEGIN\n');
 const marker='# UKE_CONTROL_SSC_'+phase+'_END\n',end=body.indexOf(marker,start);
 assert(start>0&&end>start);return body.slice(start,end+marker.length);
};
const triggerStart=body.indexOf('%transfiletriggerin -n uke-control -- /usr/lib/firmware/qcom/uke/sensors');
assert(triggerStart>0);
const triggerEnd=body.indexOf('%files\n',triggerStart);assert(triggerEnd>triggerStart);
const oldTrigger=body.slice(triggerStart,triggerEnd);
const check='bash %{_builddir}/uke-control-ssc-overlay/ssc-unit-state-test.sh %{_specdir}/uke-control.spec\n';
const changes=[
 [oldMigration('SAVE'),migration('SAVE')],
 [oldMigration('RESTORE'),migration('RESTORE')],
 [oldTrigger,b.trigger],
 [check,check+'bash %{_builddir}/uke-control-ssc-overlay/ssc-refresh-test.sh %{_specdir}/uke-control.spec\n'],
];
for(const [a,c]of changes){assert.equal(body.split(a).length,2,'Ambiguous correction');body=body.replace(a,()=>c);}
let inverse=body;
for(const [a,c]of changes.toReversed()){assert.equal(inverse.split(c).length,2,'Inverse correction');inverse=inverse.replace(c,()=>a);}
inverse=inverse.replaceAll('Release: 6%{?dist}\n','Release: 5%{?dist}\n').replaceAll('2.17.0-6','2.17.0-5');
assert.equal(inverse+history,before,'Unrelated Control5 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 2.17.0-6\n- Preserve active Adaptive targets across SSC ownership and cache refresh.\n- Respect enable and mask preferences; retain all qualified compiled sources.\n\n';
fs.writeFileSync(spec,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Control6:whole Control5 recipe inverse;only Adaptive state/cascade correction and regression coverage.');
