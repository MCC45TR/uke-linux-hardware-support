#!/usr/bin/bash
# Data-only ownership migration over the exact admitted Control215r2 cohort.
set -Eeuo pipefail
[[ $# == 1 ]]
node - "$1" <<'NODE'
const fs=require('fs'),crypto=require('crypto'),p=process.argv[2],before=fs.readFileSync(p,'utf8');
if(crypto.createHash('sha256').update(before).digest('hex')!=='2637d49b80c10f6e57dde9e36a437a657d08d83edfa1612f3848a1ed264c5a14')throw Error('Unreviewed Control parent');
const split=before.indexOf('%changelog\n');if(split<0)throw Error('Changelog boundary');let body=before.slice(0,split),history=before.slice(split);if(body.split('Version: 2.15.0\n').length!==4||body.split('Release: 2%{?dist}\n').length!==4)throw Error('Cohort version count');
body=body.replaceAll('Version: 2.15.0\n','Version: 2.16.0\n').replaceAll('Release: 2%{?dist}\n','Release: 1%{?dist}\n');const exact=(body.match(/2[.]15[.]0-2/g)||[]).length;if(exact!==12)throw Error('Cohort identity references');body=body.replaceAll('2.15.0-2','2.16.0-1');
const license='License: MIT AND Apache-2.0 AND GPL-3.0-or-later AND GPL-2.0-only\n',nextLicense=license.trimEnd()+' AND (GPL-2.0-only WITH Linux-syscall-note)\n';if(body.split('\n').filter(l=>l+'\n'===license).length!==1)throw Error('Main license');body=body.replace('\n'+license,'\n'+nextLicense);if(body.split('Source'+license).length!==2)throw Error('Source license');body=body.replace('Source'+license,'Source'+nextLicense);
const header=`# UKE_CONTROL_NFC_CONSOLIDATION
Source39: uke-control-nfc.tar.xz
Source40: fold-nfc-spec.sh
Source41: copr-control-nfc.sh
BuildRequires: acl kernel-headers
Requires: /usr/bin/ir-ctl
%ifarch aarch64
Requires: uke-kernel(nfc-ir-v2) >= 1
%endif
Provides: uke-nfc-ir = 1.1.0-3%{?dist}
Provides: uke-nfc-ir%{?_isa} = 1.1.0-3%{?dist}
Provides: uke-device-access(nfc-ir-v1) = 1
Provides: uke-nfc-tag-client(abi1) = 1
Provides: uke-control(nfc-ir-integration-v1) = 1
Obsoletes: uke-nfc-ir <= 1.1.0-2%{?dist}
`;
const prep=`mkdir -p %{_builddir}/uke-control-nfc
tar -xJf %{SOURCE39} -C %{_builddir}/uke-control-nfc
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSE NFC-MIT-LICENSE
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSES/GPL-2.0-only.txt NFC-GPL-2.0-only.txt
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSES/Linux-syscall-note NFC-Linux-syscall-note
`;
const build=`%{__cxx} %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror -I%{_builddir}/uke-control-nfc/nfc/include -I%{_builddir}/uke-control-nfc/nfc/src %{_builddir}/uke-control-nfc/nfc/src/main.cpp %{build_ldflags} -o %{_builddir}/uke-control-nfc/uke-nfc-tag
`;
const install=`install -Dm755 %{_builddir}/uke-control-nfc/uke-nfc-tag %{buildroot}/usr/bin/uke-nfc-tag
install -Dm644 %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
install -Dm644 %{_builddir}/uke-control-nfc/nfc/README.md %{buildroot}%{_docdir}/uke-control/NFC-IR.md
install -Dm644 %{_builddir}/uke-control-nfc/nfc/uapi-source.json %{buildroot}%{_docdir}/uke-control/nfc-uapi-source.json
install -Dm644 %{_builddir}/uke-control-nfc/NFC-CONSOLIDATION.md %{buildroot}%{_docdir}/uke-control/NFC-CONSOLIDATION.md
install -Dm644 %{_builddir}/uke-control-nfc/NFC-SOURCE.json %{buildroot}%{_docdir}/uke-control/NFC-SOURCE.json
`;
const check=`udevadm verify --resolve-names=never %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules
bash %{_builddir}/uke-control-nfc/nfc/tests/session-acl.sh
bash %{_builddir}/uke-control-nfc/nfc/tests/check.sh
printf '%%s  %%s\\n' "$(jq -er .sha256 %{_builddir}/uke-control-nfc/nfc/uapi-source.json)" %{_builddir}/uke-control-nfc/nfc/include/fm19511.h | sha256sum -c --strict --quiet -
readelf -h %{_builddir}/uke-control-nfc/uke-nfc-tag | grep -E 'Machine:.*(AArch64|X86-64)'
%{_builddir}/uke-control-nfc/uke-nfc-tag --help
set +e
%{_builddir}/uke-control-nfc/uke-nfc-tag read 0xffff 2 > nfc-invalid.stdout 2> nfc-invalid.stderr
nfc_status=$?
set -e
test "$nfc_status" = 2
test ! -s nfc-invalid.stdout
grep -F 'without address wrap' nfc-invalid.stderr
cmp %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
`;
const post=`# UKE_CONTROL_NFC_RULE_RELOAD
if test -d /run/udev;then
 /usr/bin/udevadm control --reload-rules >/dev/null 2>&1 || :
 /usr/bin/udevadm trigger --action=change --subsystem-match=misc --sysname-match=fm-ntag >/dev/null 2>&1 || :
 /usr/bin/udevadm trigger --action=change --subsystem-match=lirc >/dev/null 2>&1 || :
fi
`;
const files=`/usr/bin/uke-nfc-tag
/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
%{_docdir}/uke-control/NFC-IR.md
%{_docdir}/uke-control/nfc-uapi-source.json
%{_docdir}/uke-control/NFC-CONSOLIDATION.md
%{_docdir}/uke-control/NFC-SOURCE.json
%license NFC-MIT-LICENSE NFC-GPL-2.0-only.txt NFC-Linux-syscall-note
`;
const replacements=[['%description\n',header+'%description\n'],['%build\n',prep+'%build\n'+build],['%install\n','%install\n'+install],['%check\n','%check\n'+check],['%posttrans -n uke-control\n','%posttrans -n uke-control\n'+post],['%files\n','%files\n'+files]];
for(const[a,b]of replacements){if(body.split(a).length!==2)throw Error('Recipe boundary:'+a);body=body.replace(a,()=>b);}
const entry='* Tue Nov 10 2026 Uke contributors - 2.16.0-1\n- Fold the native NFC tag CLI and exact NFC/IR session rules into Control.\n- Preserve existing paths, kernel ABI, distribution IR utility and matching TUI.\n\n';
const after=body+history.replace('%changelog\n','%changelog\n'+entry);let inverse=body;for(const[a,b]of replacements.toReversed())inverse=inverse.replace(b,()=>a);inverse=inverse.replace('\n'+nextLicense,'\n'+license).replace('Source'+nextLicense,'Source'+license).replaceAll('Version: 2.16.0\n','Version: 2.15.0\n').replaceAll('Release: 1%{?dist}\n','Release: 2%{?dist}\n').replaceAll('2.16.0-1','2.15.0-2');if(inverse+history!==before)throw Error('Unrelated Control recipe or gate changed');
fs.writeFileSync(p,after);console.log('Control216 NFC recipe: complete215r2 inverse,three matching cohort versions and NFC paths/capabilities/rules retained; no service added.');
NODE
