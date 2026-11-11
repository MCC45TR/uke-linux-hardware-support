import fs from 'node:fs';
import crypto from 'node:crypto';
const [path]=process.argv.slice(2),before=fs.readFileSync(path,'utf8');
if(crypto.createHash('sha256').update(before).digest('hex')!=='3678b35b3b3514b92620b231bbd9ec368e1c46cfba5a4ae71c6f07c438fe43d3')throw Error('Unreviewed firmware14 parent');
let body=before;
const header=`Source1: retained-speaker-firmware-3.0.304.0-2.tar.xz
Provides: uke-speaker-firmware = 3.0.304.0-3%{?dist}
Provides: /usr/lib/firmware/qcom/uke/audio/fs19xx.fsm
Obsoletes: uke-speaker-firmware < 3.0.304.0-3%{?dist}
`;
const prep=`mkdir -p %{_builddir}/uke-speaker-preset
tar -xJf %{SOURCE1} --strip-components=1 -C %{_builddir}/uke-speaker-preset
`;
const install=`for item in README.md provenance.json;do install -Dm644 %{_builddir}/uke-speaker-preset/$item %{buildroot}%{_docdir}/uke-speaker-firmware/$item;done
for item in LICENSE.Apache-2.0 odm-NOTICE.xml.gz;do install -Dm644 %{_builddir}/uke-speaker-preset/$item %{buildroot}%{_licensedir}/uke-speaker-firmware/$item;done
`;
const check=`# Keep the original217-file baseline gate above; then add the exact preset.
printf 'f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b  %{_builddir}/uke-speaker-preset/fs19xx.fsm\\n' | sha256sum -c --strict -
install -Dm644 %{_builddir}/uke-speaker-preset/fs19xx.fsm %{buildroot}/usr/lib/firmware/qcom/uke/audio/fs19xx.fsm
cmp %{_builddir}/uke-speaker-preset/fs19xx.fsm %{buildroot}/usr/lib/firmware/qcom/uke/audio/fs19xx.fsm
test "$(find %{buildroot}/usr/lib/firmware %{buildroot}/usr/share/uke-sensors -type f | wc -l)" = 218
`;
const files=`%config(noreplace) /usr/lib/firmware/qcom/uke/audio/fs19xx.fsm
%dir %{_docdir}/uke-speaker-firmware
%doc %{_docdir}/uke-speaker-firmware/README.md %{_docdir}/uke-speaker-firmware/provenance.json
%dir %{_licensedir}/uke-speaker-firmware
%license %{_licensedir}/uke-speaker-firmware/LICENSE.Apache-2.0 %{_licensedir}/uke-speaker-firmware/odm-NOTICE.xml.gz
`;
const changes=[['Release: 14.fc46\n','Release: 15%{?dist}\n'],['License: LicenseRef-Proprietary\n','License: LicenseRef-Proprietary AND Apache-2.0\n'],['%description -n uke-firmware\n',header+'%description -n uke-firmware\n'],['%build\n',prep+'%build\n'],['%install\n','%install\n'+install],['%files -n uke-firmware\n',check+'%files -n uke-firmware\n'+files]];
for(const[from,to]of changes){if(body.split(from).length!==2)throw Error('Ambiguous firmware boundary');body=body.replace(from,()=>to);}
let inverse=body;for(const[from,to]of changes.toReversed()){if(inverse.split(to).length!==2)throw Error('Ambiguous inverse');inverse=inverse.replace(to,()=>from);}if(inverse!==before)throw Error('Unrelated firmware recipe changed');
fs.writeFileSync(path,body);
console.log('Firmware15 complete parent inverse;original217 checks plus exact generic preset,license/docs and noreplace flag retained.');
