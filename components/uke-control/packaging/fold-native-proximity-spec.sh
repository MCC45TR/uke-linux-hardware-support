#!/usr/bin/bash
# Data-only addition over the exact qualified Control216 cohort.
set -Eeuo pipefail
[[ $# == 1 ]]
node - "$1" <<'NODE'
const fs=require('fs'),crypto=require('crypto'),p=process.argv[2],before=fs.readFileSync(p,'utf8');
if(crypto.createHash('sha256').update(before).digest('hex')!=='16db08349320c9941a65fe909390b4c54c0fc0518807391e2535781ec0fd7cbc')throw Error('Unreviewed Control216 parent');
const split=before.indexOf('%changelog\n');if(split<0)throw Error('Missing changelog');let body=before.slice(0,split),history=before.slice(split);
if(body.split('Release: 1%{?dist}\n').length!==4||(body.match(/2[.]16[.]0-1/g)||[]).length!==12)throw Error('Parent cohort identity');
body=body.replaceAll('Release: 1%{?dist}\n','Release: 2%{?dist}\n').replaceAll('2.16.0-1','2.16.0-2');
const header=`# UKE_CONTROL_NATIVE_PROXIMITY_SELECTION
Source42: 89-uke-native-proximity.rules
Source43: NATIVE-PROXIMITY-SELECTION.md
Source44: fold-native-proximity-spec.sh
Source45: copr-control-native-proximity.sh
Source46: native-proximity-source.json
Provides: uke-control(native-proximity-selection-v1) = 1
`;
const install=`install -Dm644 %{SOURCE42} %{buildroot}/usr/lib/udev/rules.d/89-uke-native-proximity.rules
install -Dm644 %{SOURCE43} %{buildroot}%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md
`;
const check=`udevadm verify --resolve-names=never %{SOURCE42}
cmp %{SOURCE42} %{buildroot}/usr/lib/udev/rules.d/89-uke-native-proximity.rules
cmp %{SOURCE43} %{buildroot}%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md
printf 'Control native proximity selection: versioned event-discovery rule and existing216 cohort retained; no new service or physical acceptance.\\n'
`;
const trigger=' /usr/bin/udevadm trigger --action=change --subsystem-match=lirc >/dev/null 2>&1 || :\n';
const files='/usr/lib/udev/rules.d/89-uke-native-proximity.rules\n%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md\n';
const replacements=[['%description\n',header+'%description\n'],['%install\n','%install\n'+install],['%check\n','%check\n'+check],[trigger,trigger+' # Only reclassify native binary proximity; no hardware request or provider restart.\n /usr/bin/udevadm trigger --action=change --subsystem-match=iio --attr-match=name=uke-ssc-proximity >/dev/null 2>&1 || :\n'],['%files\n','%files\n'+files]];
for(const[a,b]of replacements){if(body.split(a).length!==2)throw Error('Ambiguous recipe boundary:'+a);body=body.replace(a,()=>b);}
let inverse=body;for(const[a,b]of replacements.toReversed())inverse=inverse.replace(b,()=>a);inverse=inverse.replaceAll('Release: 2%{?dist}\n','Release: 1%{?dist}\n').replaceAll('2.16.0-2','2.16.0-1');if(inverse+history!==before)throw Error('Unrelated recipe or previous test changed');
const entry='* Wed Nov 11 2026 Uke contributors - 2.16.0-2\n- Select native proximity input only on kernels declaring its versioned ABI.\n- Retain raw IIO, legacy-kernel discovery and every existing Control feature.\n\n';
fs.writeFileSync(p,body+history.replace('%changelog\n','%changelog\n'+entry));
console.log('Control216r2 proximity recipe: complete216 inverse,three matching cohort identities and event-only classification; no new runtime family.');
NODE
