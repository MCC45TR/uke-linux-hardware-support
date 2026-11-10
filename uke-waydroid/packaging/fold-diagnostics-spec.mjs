// Reuse two qualified recipes without changing their sources or runtime policy.
import fs from 'node:fs';
import crypto from 'node:crypto';
const [integrationPath, diagnosticsPath, output] = process.argv.slice(2);
const hash = x => crypto.createHash('sha256').update(x).digest('hex');
const parent = fs.readFileSync(integrationPath, 'utf8');
const diagnostic = fs.readFileSync(diagnosticsPath, 'utf8');
if (hash(parent) !== 'a0838048d75c6e40d13d41adae7b807b7624c2df6835891e3d75273986204ba8' ||
    hash(diagnostic) !== '35e3fa65cc00cb4893aa79449db23ab252030c7fa14b018b1ca35df690737f73')
  throw Error('Qualified parent recipe changed');
const edits = [];
let spec = parent;
function replace(before, after) {
  if (spec.split(before).length !== 2) throw Error('Ambiguous recipe scope: ' + before);
  spec = spec.replace(before, after);
  edits.push([before, after]);
}
function phase(name, next) {
  const start = '\n%' + name + '\n', end = '\n%' + next + '\n';
  if (diagnostic.split(start).length !== 2 || diagnostic.split(end).length !== 2)
    throw Error('Ambiguous diagnostic phase');
  return diagnostic.split(start)[1].split(end)[0].trimEnd();
}
const wrap = (name, body) => '\n# BEGIN folded Android diagnostic ' + name + '\n(\n' +
  'cd uke-waydroid-diagnostics-0.1.0\n' + body + '\n)\n' +
  '# END folded Android diagnostic ' + name + '\n';
replace('Release:        8%{?dist}', 'Release:        9%{?dist}');
replace('License:        MIT\n', 'License:        MIT AND BSD-2-Clause AND BSD-3-Clause AND Apache-2.0\n');
replace('Source0:        %{name}-%{version}.tar.xz\n',
  'Source0:        %{name}-%{version}.tar.xz\n' +
  'Source1:        uke-waydroid-diagnostics-0.1.0.tar.xz\n' +
  'Source2:        integration-parent.spec\nSource3:        diagnostics-parent.spec\n' +
  'Source4:        diagnostics-fold.json\nExclusiveArch:  aarch64\n' +
  'BuildRequires:  clang lld binutils glslang\n' +
  '%global debug_package %{nil}\n%global _build_id_links none\n' +
  '# Only this Android/Bionic ELF is excluded from host ABI dependencies.\n' +
  '%global __requires_exclude_from ^/var/lib/waydroid/overlay/system/bin/uke-vulkan-check$\n' +
  '%global __provides_exclude_from ^/var/lib/waydroid/overlay/system/bin/uke-vulkan-check$\n');
replace('Requires:       uke-waydroid-diagnostics >= 0.1.0-3\n',
  'Provides:       uke-waydroid-diagnostics = %{version}-%{release}\n' +
  'Obsoletes:      uke-waydroid-diagnostics < 0.1.0-4\n');
replace('%setup -q\n', '%setup -q\n' +
  'tar -xJf %{SOURCE1}\n' +
  'printf \'%s  %s\\n\' a0838048d75c6e40d13d41adae7b807b7624c2df6835891e3d75273986204ba8 %{SOURCE2} | sha256sum -c --strict -\n' +
  'printf \'%s  %s\\n\' 35e3fa65cc00cb4893aa79449db23ab252030c7fa14b018b1ca35df690737f73 %{SOURCE3} | sha256sum -c --strict -\n');
replace('\n%install\n', wrap('build', phase('build', 'install')) + '\n%install\n');
replace('\n%check\n', wrap('install', phase('install', 'check')) +
  'install -Dm0644 uke-waydroid-diagnostics-0.1.0/LICENSE %{buildroot}%{_datadir}/licenses/uke-waydroid-diagnostics/LICENSE\n' +
  'install -Dm0644 uke-waydroid-diagnostics-0.1.0/NDK-NOTICE %{buildroot}%{_datadir}/licenses/uke-waydroid-diagnostics/NDK-NOTICE\n' +
  'install -Dm0644 uke-waydroid-diagnostics-0.1.0/source-commit %{buildroot}%{_datadir}/doc/uke-waydroid-diagnostics/source-commit\n' +
  'install -Dm0644 uke-waydroid-diagnostics-0.1.0/ndk-source.properties %{buildroot}%{_datadir}/doc/uke-waydroid-diagnostics/ndk-source.properties\n' +
  '\n%check\n');
replace('\n%post\n', wrap('check', phase('check', 'files')) +
  'printf \'Waydroid fold: retained host tests and Android ELF checks pass; no container acceptance.\\n\'\n' +
  '\n%post\n');
replace('\n%changelog\n',
  '# Existing diagnostic paths retain their contents and gain one RPM owner.\n' +
  '%{_datadir}/licenses/uke-waydroid-diagnostics/\n' +
  '%{_datadir}/doc/uke-waydroid-diagnostics/\n' +
  '/var/lib/waydroid/overlay/system/bin/uke-vulkan-check\n' +
  '\n%changelog\n* Tue Nov 10 2026 Uke contributors - 0.1.0-9\n' +
  '- Include the unchanged manual Vulkan checker in the existing integration RPM.\n' +
  '- Preserve Android ABI, NDK notices, host policy and media preparation.\n');
let inverse = spec;
for (const [before, after] of edits.toReversed()) {
  if (inverse.split(after).length !== 2) throw Error('Ambiguous inverse');
  inverse = inverse.replace(after, before);
}
if (inverse !== parent) throw Error('Unrelated integration recipe change');
if (spec.includes('Requires:       uke-waydroid-diagnostics ')) throw Error('Self dependency');
fs.writeFileSync(output, spec);
console.log('Waydroid fold recipe: complete integration inverse; unchanged diagnostic build/install/check phases.');
