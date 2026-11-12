// SPDX-License-Identifier: MIT
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [path,contract]=process.argv.slice(2),m=JSON.parse(fs.readFileSync(contract));
assert.equal(m.result.version,'1.91.5-21.fc46');
const before=fs.readFileSync(path,'utf8');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'),m.parent.spec_sha256);
const split=before.indexOf('%changelog\n');assert(split>0);
let body=before.slice(0,split);const history=before.slice(split),changes=[];
function change(old,next){assert.equal(body.split(old).length,2,'Ambiguous library delta');body=body.replace(old,()=>next);changes.push([old,next]);}
assert.equal((body.match(/^Release: 20[.]fc46$/gm)||[]).length,4);
body=body.replaceAll('Release: 20.fc46\n','Release: 21.fc46\n').replaceAll('1.91.5-20.fc46','1.91.5-21.fc46');
change('Source0: uke-upower-1.91.5.tar.xz\n','Source0: uke-upower-1.91.5.tar.xz\nSource1: uke-upower-fedora-libs.tar.xz\nBuildRequires: upower-libs = 1.91.5-1.fc46\nBuildRequires: binutils rpm-build\n');
change('Requires:       uke-upower-libs(aarch-64) = 1.91.5-21.fc46\n','Requires: upower-libs%{?_isa} >= 1.91.5\nObsoletes: uke-upower-libs\nConflicts: uke-upower-libs\n');
const start=body.indexOf('%package -n uke-upower-libs\n'),end=body.indexOf('%prep\n');assert(start>0&&end>start);
const compatibility=`# Existing development names are metadata-only upgrade bridges.\n%package -n uke-upower-devel\nVersion: 1.91.5\nRelease: 21.fc46\nSummary: Transition to original Fedora UPower development files\nRequires: upower-devel%{?_isa} >= 1.91.5\nObsoletes: uke-upower-libs\nConflicts: uke-upower-libs\n%description -n uke-upower-devel\nKeep existing development installations functional using original Fedora files.\n\n%package -n uke-upower-devel-docs\nVersion: 1.91.5\nRelease: 21.fc46\nBuildArch: noarch\nSummary: Transition to original Fedora UPower development documentation\nRequires: upower-devel-docs >= 1.91.5\nObsoletes: uke-upower-libs\nConflicts: uke-upower-libs\n%description -n uke-upower-devel-docs\nKeep existing documentation installations functional using original Fedora files.\n\n`;
change(body.slice(start,end),compatibility);
change('%prep\n','%prep\nmkdir -p %{_builddir}/uke-upower-fedora-libs\ntar -xJf %{SOURCE1} -C %{_builddir}/uke-upower-fedora-libs\n(cd %{_builddir}/uke-upower-fedora-libs;sha256sum -c --strict --quiet SHA256SUMS)\n');
change('\n%check\n','\nbash %{_builddir}/uke-upower-fedora-libs/retire-built-library-files.sh %{buildroot}\n\n%check\nbash %{_builddir}/uke-upower-fedora-libs/check-stock-library.sh %{buildroot} %{_builddir}/uke-upower-1.91.5/work/upower/upower-v1.91.5/redhat-linux-build/libupower-glib/libupower-glib.so.3.1.2\n');
assert.equal(body.split('LD_LIBRARY_PATH=redhat-linux-build/libupower-glib ').length,4);
change('LD_LIBRARY_PATH=redhat-linux-build/libupower-glib timeout -k 5s 30s','LD_LIBRARY_PATH=/usr/lib64 timeout -k 5s 30s');
change('LD_LIBRARY_PATH=redhat-linux-build/libupower-glib timeout -k 5s 90s bash charging-fixture/tests/upower-fixture.sh redhat-linux-build/src/upowerd','LD_LIBRARY_PATH=/usr/lib64 timeout -k 5s 90s bash charging-fixture/tests/upower-fixture.sh %{buildroot}/usr/libexec/upowerd');
change('LD_LIBRARY_PATH=redhat-linux-build/libupower-glib timeout -k 5s 150s bash %{_builddir}/uke-upower-1.91.5/inputs/upower/upower-late-capacity.sh redhat-linux-build/src/upowerd','LD_LIBRARY_PATH=/usr/lib64 timeout -k 5s 150s bash %{_builddir}/uke-upower-1.91.5/inputs/upower/upower-late-capacity.sh %{buildroot}/usr/libexec/upowerd');
const filesStart=body.indexOf('%files -n uke-upower-libs\n'),filesEnd=body.indexOf('%pre -n uke-upower\n');assert(filesStart>0&&filesEnd>filesStart);
change(body.slice(filesStart,filesEnd),'# Empty bridges own no Fedora headers, libraries or documentation.\n%files -n uke-upower-devel\n\n%files -n uke-upower-devel-docs\n\n\n');
let inverse=body;
for(const[old,next]of changes.toReversed()){assert.equal(inverse.split(next).length,2,'Ambiguous library inverse');inverse=inverse.replace(next,()=>old);}
inverse=inverse.replaceAll('Release: 21.fc46\n','Release: 20.fc46\n').replaceAll('1.91.5-21.fc46','1.91.5-20.fc46');
assert.equal(inverse+history,before,'Unrelated accepted daemon recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 1.91.5-21.fc46\n- Use original Fedora client libraries without changing the necessary daemon fixes.\n- Retain existing development names only as metadata bridges to Fedora packages.\n- Run retained daemon regressions against the staged executable and Fedora library.\n\n';
fs.writeFileSync(path,body+history.replace('%changelog\n','%changelog\n'+entry));
console.log('UPower21: whole accepted20 recipe inverse passed; daemon inputs and regression cases retained.');
