// SPDX-License-Identifier: MIT
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [path, manifest] = process.argv.slice(2);
const contract = JSON.parse(fs.readFileSync(manifest));
const before = fs.readFileSync(path, 'utf8');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'), contract.parent.spec_sha256);
const split = before.indexOf('%changelog\n');
assert(split > 0);
let body = before.slice(0, split);
const history = before.slice(split), applied = [];
const change = (old, value) => {
    assert.equal(body.split(old).length, 2, 'Ambiguous SELinux recipe delta: '+old.slice(0, 60));
    body = body.replace(old, () => value);
    applied.push([old, value]);
};
assert.equal((body.match(/Release: 14%\{\?dist\}/g) || []).length, 3);
body = body.replaceAll('Release: 14%{?dist}', 'Release: 15%{?dist}').replaceAll('2.17.0-14', '2.17.0-15');
for (const field of ['SourceLicense', 'License']) {
    const old = body.split('\n').find(line => line.startsWith(field+': '));
    assert(old);
    change('\n'+old+'\n', '\n'+old+' AND GPL-2.0-or-later\n');
}
change('Source61: uke-control-pogo-integration.tar.xz\n', `Source61: uke-control-pogo-integration.tar.xz
Source62: uke-control-selinux-integration.tar.xz
Provides: uke-control(selinux-kernel-abi-v1) = 1
Requires: /usr/bin/checkpolicy /usr/bin/gawk /usr/bin/bzip2
Requires: /usr/bin/semodule /usr/bin/selinuxenabled /usr/bin/getenforce /usr/bin/load_policy
`);
change('\n%prep\n', `
%prep
selinux_overlay=%{_builddir}/uke-control-selinux-integration
mkdir "$selinux_overlay"
tar -xJf %{SOURCE62} -C "$selinux_overlay"
(cd "$selinux_overlay";sha256sum -c --strict --quiet SHA256SUMS)
`);
change('\n%check\n', `
selinux_overlay=%{_builddir}/uke-control-selinux-integration
install -Dm755 "$selinux_overlay/packaging/selinux/selinux-kernel-abi.sh" %{buildroot}/usr/libexec/uke-control/selinux-kernel-abi
install -Dm644 "$selinux_overlay/packaging/selinux/compose-kernel-abi.awk" %{buildroot}/usr/libexec/uke-control/compose-kernel-abi.awk
install -Dm644 "$selinux_overlay/docs/SELINUX-ORIGINAL.md" %{buildroot}%{_docdir}/uke-control/SELINUX-ORIGINAL.md

%check
bash %{_builddir}/uke-control-selinux-integration/packaging/check-selinux-integration.sh %{_builddir}/uke-control-selinux-integration %{buildroot} %{_specdir}/uke-control.spec
`);
const helper='/usr/libexec/uke-control/selinux-kernel-abi';
const paths='/var/lib/selinux/targeted/active/modules/100 /var/lib/selinux/minimum/active/modules/100 /var/lib/selinux/mls/active/modules/100';
change('\n%files\n', `
# Capture before RPM replaces policy.kern; initial installation uses live policy.
%transfiletriggerun -n uke-control -P 2000000 -- ${paths}
if [ -x ${helper} ]; then ${helper} capture || exit $?; fi

# Stage before the original policy package runs its standard scriptlets.
%filetriggerin -n uke-control -P 2000000 -- ${paths} ${helper}
if [ -x ${helper} ]; then ${helper} stage || exit $?; fi

# Rebuild after all policy producers and load only the selected live profile.
%transfiletriggerin -n uke-control -P 90000 -- ${paths} /usr/share/selinux/packages ${helper}
if [ -x ${helper} ]; then ${helper} finish || exit $?; fi

%files
/usr/libexec/uke-control/selinux-kernel-abi
/usr/libexec/uke-control/compose-kernel-abi.awk
%doc %{_docdir}/uke-control/SELINUX-ORIGINAL.md
`);
let inverse = body;
for (const [old, value] of applied.toReversed()) {
    assert.equal(inverse.split(value).length, 2, 'Ambiguous SELinux recipe inverse');
    inverse = inverse.replace(value, () => old);
}
inverse = inverse.replaceAll('Release: 15%{?dist}', 'Release: 14%{?dist}').replaceAll('2.17.0-15', '2.17.0-14');
assert.equal(inverse+history, before, 'Unrelated Control14 recipe changed');
const entry='* Thu Nov 12 2026 Uke contributors - 2.17.0-15\n- Compose native kernel ABI definitions with the original Fedora SELinux policy.\n- Preserve original policy sources, local module preferences and enforcing state.\n\n';
const after=body+history.replace('%changelog\n', '%changelog\n'+entry);
if (contract.result.spec_sha256) assert.equal(crypto.createHash('sha256').update(after).digest('hex'), contract.result.spec_sha256);
fs.writeFileSync(path, after);
console.log('Control15: complete Control14 recipe inverse passes; only declared SELinux integration changes.');
