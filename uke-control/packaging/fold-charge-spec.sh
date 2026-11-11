#!/usr/bin/bash
# Data-only transformation over the actual accepted Control216r2 recipe.
set -Eeuo pipefail
[[ $# == 2 ]]
node - "$1" "$2" <<'NODE'
const fs=require('fs'),crypto=require('crypto'),[p,statePath]=process.argv.slice(2),before=fs.readFileSync(p,'utf8');
if(crypto.createHash('sha256').update(before).digest('hex')!=='7572bff2614d7a6ae8837a7655f62149b06d42a5b0d7f157581aa7b8fe21ea4b')throw Error('Unreviewed Control216r2 parent');
const split=before.indexOf('%changelog\n');if(split<0)throw Error('Missing changelog');let body=before.slice(0,split),history=before.slice(split);
if(body.split('Version: 2.16.0\n').length!==4||body.split('Release: 2%{?dist}\n').length!==4||(body.match(/2[.]16[.]0-2/g)||[]).length!==12)throw Error('Parent cohort identities');
body=body.replaceAll('Version: 2.16.0\n','Version: 2.17.0\n').replaceAll('Release: 2%{?dist}\n','Release: 1%{?dist}\n').replaceAll('2.16.0-2','2.17.0-1');
const header=`# UKE_CONTROL_CHARGE_OWNERSHIP
Source47: uke-control-charge.tar.xz
Source48: fold-charge-spec.sh
Source49: copr-control-charge-fold.sh
Source50: charge-fold-source.json
Requires: upower >= 1.91.5
Provides: uke-charge-control = 1.4.1-8%{?dist}
Provides: uke-charge-control%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-charge-control < 1.4.1-8%{?dist}
Provides: uke-control(charge-bridge-v1) = 1
`;
const prep=`mkdir -p %{_builddir}/uke-control-charge
tar -xJf %{SOURCE47} -C %{_builddir}/uke-control-charge
bash %{_builddir}/uke-control-charge/packaging/apply-charge-overlay.sh "$curve_control" %{_builddir}/uke-control-charge
install -m644 %{_builddir}/uke-control-charge/CHARGE-CONSOLIDATION.md CHARGE-CONSOLIDATION.md
`;
const build=`make -C %{_builddir}/uke-control-charge/runtime CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" %{_builddir}/uke-control-charge/runtime/build/uke-charge-control
readelf -h %{_builddir}/uke-control-charge/runtime/build/uke-charge-control | grep -F AArch64
`;
const install=`charge_runtime=%{_builddir}/uke-control-charge/runtime
install -Dm755 "$charge_runtime/build/uke-charge-control" %{buildroot}%{_libexecdir}/uke-charge-control
install -Dm644 "$charge_runtime/manifests/protection-readiness.json" %{buildroot}%{_datadir}/uke-battery/protection-readiness.json
for unit in control automatic protection;do
 install -Dm644 "$charge_runtime/packaging/systemd/uke-charge-$unit.service" %{buildroot}%{_unitdir}/uke-charge-$unit.service
done
install -Dm644 "$charge_runtime/packaging/udev/80-uke-charge-protection.rules" %{buildroot}%{_udevrulesdir}/80-uke-charge-protection.rules
install -Dm644 "$charge_runtime/packaging/systemd/91-uke-charge-control.preset" %{buildroot}%{_presetdir}/91-uke-charge-control.preset
install -Dm644 "$charge_runtime/manifests/control-profile.json" %{buildroot}%{_sysconfdir}/uke-battery/control-profile.json
install -Dm644 "$charge_runtime/packaging/upower/60-uke-battery-safety.conf" %{buildroot}%{_sysconfdir}/UPower/UPower.conf.d/60-uke-battery-safety.conf
for doc in CHARGE-CONTROL.md CHARGING-PROTOCOLS.md BATTERY-LONGEVITY.md CHARGE-PUMP-PROTECTION.md OSS-CHARGING-INVENTORY.tsv CHARGING-DOCUMENTS.tsv;do
 install -Dm644 "$charge_runtime/docs/$doc" %{buildroot}%{_docdir}/uke-charge-control/$doc
done
install -Dm644 "$charge_runtime/LICENSE" %{buildroot}%{_licensedir}/uke-charge-control/LICENSE
`;
const check=`bash %{_builddir}/uke-control-charge/tests/charge-unit-state.sh %{_specdir}/uke-control.spec
%{_builddir}/uke-control-charge/runtime/build/uke-charge-control --help | grep -F apply-cap80-online
printf 'Control charge ownership: native91 source delta,helper and retained unit/config payload; no electrical or event-monitor parity claim.\\n'
`;
const files=`%{_libexecdir}/uke-charge-control
%{_datadir}/uke-battery/protection-readiness.json
%{_unitdir}/uke-charge-control.service
%{_unitdir}/uke-charge-automatic.service
%{_unitdir}/uke-charge-protection.service
%{_udevrulesdir}/80-uke-charge-protection.rules
%{_presetdir}/91-uke-charge-control.preset
%config(noreplace) %{_sysconfdir}/uke-battery/control-profile.json
%config(noreplace) %{_sysconfdir}/UPower/UPower.conf.d/60-uke-battery-safety.conf
%dir %{_docdir}/uke-charge-control
%doc %{_docdir}/uke-charge-control/*
%dir %{_licensedir}/uke-charge-control
%license %{_licensedir}/uke-charge-control/LICENSE
%doc CHARGE-CONSOLIDATION.md
`;
const state=fs.readFileSync(statePath,'utf8').replaceAll('%','%%');
const fragment=phase=>`(\n# UKE_CONTROL_CHARGE_${phase.toUpperCase()}_BEGIN\nphase=${phase}\n${state}# UKE_CONTROL_CHARGE_${phase.toUpperCase()}_END\n) || exit $?\n`;
const units='uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service';
const replacements=[['%description\n',header+'%description\n'],['%build\n',prep+'%build\n'+build],['%install\n','%install\n'+install],['%check\n','%check\n'+check],['%pre -n uke-control\n','%pre -n uke-control\n'+fragment('save')],['%posttrans -n uke-control\n','%posttrans -n uke-control\n'+fragment('restore')],['%post -n uke-control\n','%post -n uke-control\n%systemd_post '+units+'\n'],['%preun -n uke-control\n','%preun -n uke-control\n%systemd_preun '+units+'\n'],['%postun -n uke-control\n','%postun -n uke-control\n%systemd_postun '+units+'\n'],['%files\n','%files\n'+files]];
for(const[a,b]of replacements){if(body.split(a).length!==2)throw Error('Ambiguous recipe boundary:'+a);body=body.replace(a,()=>b);}
let inverse=body;for(const[a,b]of replacements.toReversed()){if(inverse.split(b).length!==2)throw Error('Ambiguous inverse');inverse=inverse.replace(b,()=>a);}inverse=inverse.replaceAll('Version: 2.17.0\n','Version: 2.16.0\n').replaceAll('Release: 1%{?dist}\n','Release: 2%{?dist}\n').replaceAll('2.17.0-1','2.16.0-2');if(inverse+history!==before)throw Error('Unrelated Control recipe or gate changed');
const entry='* Wed Nov 11 2026 Uke contributors - 2.17.0-1\n- Fold the existing charge helper,units and config ownership into Control.\n- Add the explicit fixed online cap bridge and retain all prior features.\n- Preserve user unit/profile state and the original zero-argument client ABI.\n\n';fs.writeFileSync(p,body+history.replace('%changelog\n','%changelog\n'+entry));
console.log('Control217 charge recipe: complete216r2 inverse,three matching cohort names,old helper compatibility and retained state/config ownership; no runtime acceptance.');
NODE
