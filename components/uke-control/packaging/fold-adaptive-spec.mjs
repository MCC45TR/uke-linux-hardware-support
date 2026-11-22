import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [specPath,contractPath,statePath]=process.argv.slice(2);
const m=JSON.parse(fs.readFileSync(contractPath)),before=fs.readFileSync(specPath,'utf8');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'),m.control.spec_sha256);
assert.equal(m.result.version,'2.17.0-4');assert.equal(m.result.new_runtime_packages,0);
assert.deepEqual(m.result.runtime_names,['uke-control','uke-control-client-devel','uke-control-tui']);
assert.equal(m.dependency_scope.filter_path,'^/usr/lib64/qt6/plugins/ksystemstats/libuke_adreno[.]so$');
assert.equal(m.dependency_scope.elf_consumer,'ksystemstats');assert.equal(m.dependency_scope.desktop_consumer,'plasma-workspace');
assert.equal(m.plugin_elf_requires.length,19);assert.equal(new Set(m.plugin_elf_requires).size,19);
for(const cap of m.plugin_elf_requires)assert(/^[A-Za-z0-9_+().-]+$/.test(cap),'Invalid ELF capability');
const split=before.indexOf('%changelog\n');assert(split>=0);
let body=before.slice(0,split);const history=before.slice(split);
assert.equal(body.split('Release: 3%{?dist}\n').length,4);
assert.equal((body.match(/2[.]17[.]0-3/g)||[]).length,12);
assert(!body.includes('__requires_exclude_from'));
body=body.replaceAll('Release: 3%{?dist}\n','Release: 4%{?dist}\n').replaceAll('2.17.0-3','2.17.0-4');
const header=`# UKE_CONTROL_ADAPTIVE_OWNERSHIP
Source55: uke-control-adaptive-services.tar.xz
BuildRequires: cmake ninja-build extra-cmake-modules
BuildRequires: qt6-qtbase-devel kf6-kcoreaddons-devel libksysguard-devel lm_sensors-devel
Provides: uke-adaptive-display = 1.0.16-18%{?dist}
Provides: uke-adaptive-display%{?_isa} = 1.0.16-18%{?dist}
Provides: config(uke-adaptive-display) = 1.0.16-18%{?dist}
Obsoletes: uke-adaptive-display < 1.0.16-18%{?dist}
Provides: uke-control(adaptive-integration-v1) = 1
Requires: iio-sensor-proxy(ssc) >= 1
Requires: uke-ssc-fileservice >= 2026.09.21-8
Requires: coreutils grep sed systemd-udev
Requires: ddcutil >= 3.0.2
Requires: libddcutil >= 3.0.2
Requires: systemd >= 256
Requires: (powerdevil >= 6.8.0 if plasma-workspace)
Requires: (kscreen >= 6.8.0 if plasma-workspace)
Requires: (ksystemstats >= 6.8.0 if plasma-workspace)
Requires: (libksysguard >= 6.8.0 if ksystemstats)
# Only the optional project plugin; all its ELF requirements are explicit below.
%global __requires_exclude_from ^%{_libdir}/qt6/plugins/ksystemstats/libuke_adreno[.]so$
${m.plugin_elf_requires.map(x=>'Requires: ('+x+' if ksystemstats)').join('\n')}
`;
const prep=`mkdir -p %{_builddir}/uke-control-adaptive-overlay
tar -xJf %{SOURCE55} -C %{_builddir}/uke-control-adaptive-overlay
(cd %{_builddir}/uke-control-adaptive-overlay;sha256sum -c --strict --quiet SHA256SUMS)
bash %{_builddir}/uke-control-adaptive-overlay/prepare-adaptive-sources.sh %{_builddir}/uke-control-2.8.1 %{_builddir}/uke-control-adaptive %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json
install -m644 %{_builddir}/uke-control-adaptive-overlay/ADAPTIVE-SERVICES.md ADAPTIVE-SERVICES.md
`;
const build=`(
cd %{_builddir}/uke-control-adaptive/runtime
%set_build_flags
cmake -S src/adreno-stats -B redhat-linux-build -G Ninja \\
 -DCMAKE_C_FLAGS_RELEASE:STRING=-DNDEBUG -DCMAKE_CXX_FLAGS_RELEASE:STRING=-DNDEBUG \\
 -DCMAKE_Fortran_FLAGS_RELEASE:STRING=-DNDEBUG -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \\
 -DCMAKE_INSTALL_DO_STRIP:BOOL=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr \\
 -DCMAKE_INSTALL_FULL_SBINDIR:PATH=/usr/bin -DCMAKE_INSTALL_SBINDIR:PATH=bin \\
 -DINCLUDE_INSTALL_DIR:PATH=/usr/include -DLIB_INSTALL_DIR:PATH=/usr/lib64 \\
 -DSYSCONF_INSTALL_DIR:PATH=/etc -DSHARE_INSTALL_PREFIX:PATH=/usr/share \\
 -DLIB_SUFFIX=64 -DBUILD_SHARED_LIBS:BOOL=ON
cmake --build redhat-linux-build -j"$RPM_BUILD_NCPUS" --verbose
bash %{_builddir}/uke-control-adaptive-overlay/check-adaptive-plugin-deps.sh \\
 "$PWD/redhat-linux-build/libuke_adreno.so" %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json \\
 %{_builddir}/uke-control-adaptive/plugin-elf.requires
)
`;
const install=`adaptive=%{_builddir}/uke-control-adaptive/runtime
DESTDIR=%{buildroot} cmake --install "$adaptive/redhat-linux-build"
for item in adreno.page;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/usr/share/plasma-systemmonitor/$item;done
install -Dm644 "$adaptive/packaging/uke-adaptive-display.target" %{buildroot}/usr/lib/systemd/system/uke-adaptive-display.target
install -Dm644 "$adaptive/packaging/80-uke-adaptive-display.preset" %{buildroot}/usr/lib/systemd/system-preset/80-uke-adaptive-display.preset
for item in 71-uke-tablet-mode.rules 61-uke-display-ddc.rules;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/usr/lib/udev/rules.d/$item;done
install -Dm644 "$adaptive/packaging/60-block-scheduler.rules" %{buildroot}/etc/udev/rules.d/60-block-scheduler.rules
for item in powerdevilrc kwinoutputconfig.json;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/etc/xdg/$item;done
install -Dm755 "$adaptive/src/uke-ddc-config" %{buildroot}/usr/libexec/senemos-uke/uke-ddc-config
install -Dm644 "$adaptive/packaging/60-uke-display-ddc.conf" %{buildroot}/usr/lib/systemd/user/plasma-powerdevil.service.d/60-uke-display-ddc.conf
install -Dm644 "$adaptive/packaging/30-uke-s2idle.conf" %{buildroot}/usr/lib/systemd/sleep.conf.d/30-uke-s2idle.conf
install -Dm644 "$adaptive/packaging/uke-suspend.conf" %{buildroot}/usr/lib/tmpfiles.d/uke-suspend.conf
for item in ADAPTIVE-DISPLAY.md DISPLAY-CONTROLS.md SUSPEND.md SERVICE-POLICY.md GPU-MONITORING.md;do
 install -Dm644 "$adaptive/docs/$item" %{buildroot}/usr/share/doc/uke-adaptive-display/$item
done
install -Dm644 "$adaptive/LICENSE" %{buildroot}/usr/share/licenses/uke-adaptive-display/LICENSE
`;
const check=`adaptive=%{_builddir}/uke-control-adaptive/runtime
ctest --test-dir "$adaptive/redhat-linux-build" --output-on-failure --force-new-ctest-process -j"$RPM_BUILD_NCPUS"
for item in 71-uke-tablet-mode.rules 61-uke-display-ddc.rules 60-block-scheduler.rules;do udevadm verify "$adaptive/packaging/$item";done
(cd "$adaptive";sh tests/ddc-config.sh)
systemd-tmpfiles --dry-run --create "$adaptive/packaging/uke-suspend.conf"
bash %{_builddir}/uke-control-adaptive-overlay/adaptive-unit-state-test.sh %{_specdir}/uke-control.spec
bash %{_builddir}/uke-control-adaptive-overlay/check-adaptive-plugin-deps.sh \\
 %{buildroot}/usr/lib64/qt6/plugins/ksystemstats/libuke_adreno.so \\
 %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json \\
 %{_builddir}/uke-control-adaptive/installed-plugin-elf.requires
printf 'Adaptive17 native plugin,parser,DDC,udev,tmpfiles and transaction checks retained;no physical display acceptance implied.\\n'
`;
const files=`/usr/lib64/qt6/plugins/ksystemstats/libuke_adreno.so
/usr/share/plasma-systemmonitor/adreno.page
%config(noreplace) /etc/xdg/powerdevilrc
%config(noreplace) /etc/xdg/kwinoutputconfig.json
%config(noreplace) /etc/udev/rules.d/60-block-scheduler.rules
/usr/lib/systemd/system/uke-adaptive-display.target
/usr/lib/systemd/system-preset/80-uke-adaptive-display.preset
/usr/lib/udev/rules.d/71-uke-tablet-mode.rules
/usr/lib/udev/rules.d/61-uke-display-ddc.rules
/usr/libexec/senemos-uke/uke-ddc-config
/usr/lib/systemd/user/plasma-powerdevil.service.d/60-uke-display-ddc.conf
/usr/lib/systemd/sleep.conf.d/30-uke-s2idle.conf
/usr/lib/tmpfiles.d/uke-suspend.conf
%dir /usr/share/doc/uke-adaptive-display
%doc /usr/share/doc/uke-adaptive-display/*
%dir /usr/share/licenses/uke-adaptive-display
%license /usr/share/licenses/uke-adaptive-display/LICENSE
%doc ADAPTIVE-SERVICES.md
`;
const state=fs.readFileSync(statePath,'utf8').replaceAll('%','%%');
const fragment=phase=>`(\n# UKE_CONTROL_ADAPTIVE_${phase.toUpperCase()}_BEGIN\nphase=${phase}\n${state}# UKE_CONTROL_ADAPTIVE_${phase.toUpperCase()}_END\n) || exit $?\n`;
const changes=[['%description\n',header+'%description\n'],['%build\n',prep+'%build\n'+build],['%install\n','%install\n'+install],['%check\n','%check\n'+check],['%pre -n uke-control\n','%pre -n uke-control\n'+fragment('save')],['%posttrans -n uke-control\n','%posttrans -n uke-control\n'+fragment('restore')],['%post -n uke-control\n','%post -n uke-control\n%systemd_post uke-adaptive-display.target\ncommand -v systemd-tmpfiles >/dev/null && systemd-tmpfiles --create /usr/lib/tmpfiles.d/uke-suspend.conf || :\n'],['%preun -n uke-control\n','%preun -n uke-control\n%systemd_preun uke-adaptive-display.target\n'],['%postun -n uke-control\n','%postun -n uke-control\n%systemd_postun uke-adaptive-display.target\n'],['%files\n','%files\n'+files]];
for(const[a,b]of changes){assert.equal(body.split(a).length,2,'Ambiguous anchor:'+a);body=body.replace(a,()=>b);}
let inverse=body;
for(const[a,b]of changes.toReversed()){assert.equal(inverse.split(b).length,2,'Ambiguous inverse');inverse=inverse.replace(b,()=>a);}
inverse=inverse.replaceAll('Release: 4%{?dist}\n','Release: 3%{?dist}\n').replaceAll('2.17.0-4','2.17.0-3');
assert.equal(inverse+history,before,'Unrelated parent recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 2.17.0-4\n- Retain Adaptive17 project plugin and policy in the existing Control package.\n- Preserve configuration and target preferences across ownership migration.\n- Keep original KDE dependencies conditional on their consumers.\n\n';
fs.writeFileSync(specPath,body+history.replace('%changelog\n',()=>'%changelog\n'+entry));
console.log('Control217r4 recipe: whole parent inverse,Adaptive17 checks/configs/target retained,one exact plugin dependency scope and three existing output names.');
