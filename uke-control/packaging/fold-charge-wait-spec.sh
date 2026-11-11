#!/usr/bin/bash
# Keep the actual accepted recipe and its full checks; change the RPM release.
set -Eeuo pipefail
[[ $# == 1 ]]
node - "$1" <<'NODE'
const fs=require('fs'),assert=require('assert'),crypto=require('crypto'),p=process.argv[2],before=fs.readFileSync(p,'utf8');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'),'a96361c448efcd3c2809041e10a1bc5fda5c4196d82acb6005f9a76594cd01d0');
const split=before.indexOf('%changelog\n');assert(split>=0);let body=before.slice(0,split);const history=before.slice(split);
assert.equal(body.split('Version: 2.17.0\n').length,4);assert.equal(body.split('Release: 1%{?dist}\n').length,4);assert.equal(body.split('2.17.0-1').length,13);
body=body.replaceAll('Release: 1%{?dist}\n','Release: 2%{?dist}\n').replaceAll('2.17.0-1','2.17.0-2');
const header='# UKE_CONTROL_CHARGE_WAIT\nSource51: uke-control-charge-wait.tar.xz\n';
const prep=`mkdir -p %{_builddir}/uke-control-charge-wait
tar -xJf %{SOURCE51} -C %{_builddir}/uke-control-charge-wait
bash %{_builddir}/uke-control-charge-wait/apply-charge-wait.sh %{_builddir}/uke-control-charge/runtime %{_builddir}/uke-control-charge-wait
install -m644 %{_builddir}/uke-control-charge-wait/CHARGE-WAIT.md CHARGE-WAIT.md
`;
const check=`bash %{_builddir}/uke-control-charge-wait/check-charge-wait-native.sh %{_builddir}/uke-control-charge/runtime
printf 'Control charge wait: actual helper,same protection interval and retained runtime/config ownership; physical power acceptance remains open.\\n'
`;
const replacements=[['%description\n',header+'%description\n'],['%build\n',prep+'%build\n'],['%check\n','%check\n'+check],['%files\n','%files\n%doc CHARGE-WAIT.md\n']];
for(const [a,b]of replacements){assert.equal(body.split(a).length,2);body=body.replace(a,()=>b);}
let inverse=body;for(const [a,b]of replacements.toReversed()){assert.equal(inverse.split(b).length,2);inverse=inverse.replace(b,()=>a);}
inverse=inverse.replaceAll('Release: 2%{?dist}\n','Release: 1%{?dist}\n').replaceAll('2.17.0-2','2.17.0-1');assert.equal(inverse+history,before);
const entry='* Wed Nov 11 2026 Uke contributors - 2.17.0-2\n- Replace short periodic wait ticks with one interruptible deadline.\n- Preserve the protection interval,helper ownership and persistent settings.\n\n';
fs.writeFileSync(p,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Control217r2 recipe: full parent inverse,three existing names and retained checks,units,config and dependencies.');
NODE
