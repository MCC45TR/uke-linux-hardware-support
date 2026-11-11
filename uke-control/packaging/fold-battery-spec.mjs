import fs from 'node:fs';
import crypto from 'node:crypto';

const [specPath,statePath]=process.argv.slice(2);
const before=fs.readFileSync(specPath,'utf8');
if(crypto.createHash('sha256').update(before).digest('hex')!=='79c486669adb6d1feae8a4781940f86367f91c650bf4f99a7eb6b7947fe010df')throw Error('Unreviewed Control217r2 parent');
const split=before.indexOf('%changelog\n');
if(split<0)throw Error('Missing changelog');
let body=before.slice(0,split);const history=before.slice(split);
if(body.split('Release: 2%{?dist}\n').length!==4||(body.match(/2[.]17[.]0-2/g)||[]).length!==12)throw Error('Unexpected cohort identities');
body=body.replaceAll('Release: 2%{?dist}\n','Release: 3%{?dist}\n').replaceAll('2.17.0-2','2.17.0-3');
const header=`# UKE_CONTROL_BATTERY_OWNERSHIP
Source52: retained-battery-services-1.4.1-7.tar.xz
Source53: uke-control-battery-services.tar.xz
Source54: uke-battery.conf
Provides: uke-battery-observer = 1.4.1-8%{?dist}
Provides: uke-battery-observer%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-battery-observer < 1.4.1-8%{?dist}
Provides: uke-offline-charge = 1.4.1-8%{?dist}
Provides: uke-offline-charge%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-offline-charge < 1.4.1-8%{?dist}
Provides: uke-control(battery-services-v1) = 1
`;
const prep=`mkdir -p %{_builddir}/uke-control-battery/runtime %{_builddir}/uke-control-battery/overlay
tar -xJf %{SOURCE52} --strip-components=1 -C %{_builddir}/uke-control-battery/runtime
(cd %{_builddir}/uke-control-battery/runtime;sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
tar -xJf %{SOURCE53} -C %{_builddir}/uke-control-battery/overlay
(cd %{_builddir}/uke-control-battery/overlay;sha256sum -c --strict --quiet SHA256SUMS)
install -m644 %{_builddir}/uke-control-battery/overlay/BATTERY-SERVICES.md BATTERY-SERVICES.md
`;
const build=`battery_runtime=%{_builddir}/uke-control-battery/runtime
make -C "$battery_runtime" CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" "$battery_runtime/build/uke-battery" "$battery_runtime/build/uke-offline-charge"
for binary in uke-battery uke-offline-charge;do readelf -h "$battery_runtime/build/$binary" | grep -F AArch64;done
`;
const install=`battery_runtime=%{_builddir}/uke-control-battery/runtime
install -Dm755 "$battery_runtime/build/uke-battery" %{buildroot}%{_bindir}/uke-battery
install -Dm755 "$battery_runtime/build/uke-offline-charge" %{buildroot}%{_libexecdir}/uke-offline-charge
for unit in uke-battery.service uke-offline-charge.service uke-offline-charge.target;do
 install -Dm644 "$battery_runtime/packaging/systemd/$unit" %{buildroot}%{_unitdir}/$unit
done
install -Dm644 "$battery_runtime/packaging/systemd/90-uke-battery.preset" %{buildroot}%{_presetdir}/90-uke-battery.preset
install -Dm644 %{SOURCE54} %{buildroot}%{_sysusersdir}/uke-battery.conf
for item in org.senemos.Uke.Battery1.conf org.senemos.Uke.Battery1.service;do
 case $item in *.conf) directory=system.d;; *.service) directory=system-services;; esac
 install -Dm644 "$battery_runtime/packaging/dbus/$item" %{buildroot}%{_datadir}/dbus-1/$directory/$item
done
for item in battery-profile.json model-profile.json;do install -Dm644 "$battery_runtime/manifests/$item" %{buildroot}%{_datadir}/uke-battery/$item;done
for item in README.md SOURCE-SHA256SUMS pack-variant-source.json;do install -Dm644 "$battery_runtime/$item" %{buildroot}%{_docdir}/uke-battery-observer/$item;done
for item in BATTERY.md RUNTIME-PROFILE.md BATTERY-ENGINEERING.md;do install -Dm644 "$battery_runtime/docs/$item" %{buildroot}%{_docdir}/uke-battery-observer/$item;done
install -Dm644 "$battery_runtime/docs/OFFLINE-CHARGING.md" %{buildroot}%{_docdir}/uke-offline-charge/OFFLINE-CHARGING.md
for package in uke-battery-observer uke-offline-charge;do install -Dm644 "$battery_runtime/LICENSE" %{buildroot}%{_licensedir}/$package/LICENSE;done
`;
const check=`make -C %{_builddir}/uke-control-battery/runtime CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" validate test-native
bash %{_builddir}/uke-control-battery/overlay/battery-unit-state-test.sh %{_specdir}/uke-control.spec
printf 'Battery/offline original native checks retained; no physical charger-boot acceptance implied.\\n'
`;
const files=`%{_bindir}/uke-battery
%{_libexecdir}/uke-offline-charge
%{_unitdir}/uke-battery.service
%{_unitdir}/uke-offline-charge.service
%{_unitdir}/uke-offline-charge.target
%{_presetdir}/90-uke-battery.preset
%{_sysusersdir}/uke-battery.conf
%{_datadir}/dbus-1/system.d/org.senemos.Uke.Battery1.conf
%{_datadir}/dbus-1/system-services/org.senemos.Uke.Battery1.service
%{_datadir}/uke-battery/battery-profile.json
%{_datadir}/uke-battery/model-profile.json
%dir %{_docdir}/uke-battery-observer
%doc %{_docdir}/uke-battery-observer/*
%dir %{_docdir}/uke-offline-charge
%doc %{_docdir}/uke-offline-charge/*
%dir %{_licensedir}/uke-battery-observer
%license %{_licensedir}/uke-battery-observer/LICENSE
%dir %{_licensedir}/uke-offline-charge
%license %{_licensedir}/uke-offline-charge/LICENSE
%doc BATTERY-SERVICES.md
`;
const state=fs.readFileSync(statePath,'utf8').replaceAll('%','%%');
const fragment=phase=>`(\n# UKE_CONTROL_BATTERY_${phase.toUpperCase()}_BEGIN\nphase=${phase}\n${state}# UKE_CONTROL_BATTERY_${phase.toUpperCase()}_END\n) || exit $?\n`;
const changes=[['Recommends: uke-battery-observer\n',''],['%description\n',header+'%description\n'],['%build\n',prep+'%build\n'+build],['%install\n','%install\n'+install],['%check\n','%check\n'+check],['%pre -n uke-control\n','%pre -n uke-control\n%sysusers_create_compat %{SOURCE54}\n'+fragment('save')],['%posttrans -n uke-control\n','%posttrans -n uke-control\n'+fragment('restore')],['%post -n uke-control\n','%post -n uke-control\n%systemd_post uke-battery.service\n'],['%preun -n uke-control\n','%preun -n uke-control\n%systemd_preun uke-battery.service\n'],['%postun -n uke-control\n','%postun -n uke-control\n%systemd_postun uke-battery.service\n'],['%files\n','%files\n'+files]];
for(const [from,to]of changes){if(body.split(from).length!==2)throw Error('Ambiguous boundary:'+from);body=body.replace(from,()=>to);}
let inverse=body;
for(const[from,to]of changes.toReversed()){
 if(!to){const anchor='Recommends: /usr/lib64/spa-0.2/aec/libspa-aec-webrtc.so\n';if(inverse.split(anchor).length!==2)throw Error('Recommends inverse');inverse=inverse.replace(anchor,()=>from+anchor);}
 else{if(inverse.split(to).length!==2)throw Error('Ambiguous inverse');inverse=inverse.replace(to,()=>from);}
}
inverse=inverse.replaceAll('Release: 3%{?dist}\n','Release: 2%{?dist}\n').replaceAll('2.17.0-3','2.17.0-2');
if(inverse+history!==before)throw Error('Unrelated Control recipe changed');
const entry='* Wed Nov 11 2026 Uke contributors - 2.17.0-3\n- Consolidate unchanged battery observation and offline UI into Control.\n- Preserve battery activation preferences and the current charge helper.\n\n';
fs.writeFileSync(specPath,body+history.replace('%changelog\n','%changelog\n'+entry));
console.log('Control217r3 recipe: whole parent inverse,original battery/offline sources and checks retained;three existing outputs.');
