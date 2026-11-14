// SPDX-License-Identifier: MIT
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [path, manifest, owner] = process.argv.slice(2);
assert(path && manifest && owner);
const contract = JSON.parse(fs.readFileSync(manifest));
const before = fs.readFileSync(path, 'utf8');
const digest = value => crypto.createHash('sha256').update(value).digest('hex');
assert.equal(digest(before), contract.parent.spec_sha256);
assert.equal(contract.release, 19);
const release = contract.release;
assert.equal(contract.production_admitted, false);
assert.equal(contract.parent.nvr, 'uke-control-2.17.0-16');
assert.equal(contract.parent.non_spec_sources, 64);
const split = before.indexOf('%changelog\n');
assert(split > 0);
let body = before.slice(0, split);
const history = before.slice(split), applied = [];
const change = (old, value) => {
    assert.equal(body.split(old).length, 2, 'Ambiguous Motion recipe delta: '+old.slice(0, 60));
    body = body.replace(old, () => value);
    applied.push([old, value]);
};
assert.equal(body.split('Release: 16%{?dist}').length, 4);
const tuples = body.split('\n').filter(line => line.includes('2.17.0-16'));
assert.equal(tuples.length, 12);
assert(tuples.every(line => /^(Requires|Recommends|Provides|Obsoletes):/.test(line)));
assert(!body.includes('Source64:') && !body.includes('Source65:'));
body = body.replaceAll('Release: 16%{?dist}', `Release: ${release}%{?dist}`).replaceAll('2.17.0-16', `2.17.0-${release}`);
change('Source63: uke-control-closed-cover-health.tar.xz\n', `Source63: uke-control-closed-cover-health.tar.xz
Source64: uke-control-motion.tar.xz
Source65: original-fedora-motion-policy.rpm
BuildRequires: checkpolicy policycoreutils cpio bzip2
Provides: uke-control(motion1-v1) = 1
Requires(post): /usr/bin/restorecon
`);
change('\n%prep\n', `
%prep
motion_overlay=%{_builddir}/uke-control-motion
mkdir "$motion_overlay"
tar -xJf %{SOURCE64} -C "$motion_overlay"
(cd "$motion_overlay";sha256sum -c --strict --quiet SHA256SUMS;node uke-control/packaging/motion-assets.mjs --check)
`);
change('\n%build\n', `
%build
(
%set_build_flags
cd %{_builddir}/uke-control-motion/uke-control
make -f packaging/motion.mk CXXFLAGS="$CXXFLAGS -std=c++20 -Wall -Wextra -Werror" LDFLAGS="$LDFLAGS" all
)
`);
change('\n%install\n', `
%install
motion=%{_builddir}/uke-control-motion/uke-control
install -Dm755 "$motion/build/motion/motion" %{buildroot}/usr/libexec/uke-control/motion
install -Dm755 "$motion/packaging/selinux/motion-policy.sh" %{buildroot}/usr/libexec/uke-control/motion-policy
install -Dm644 "$motion/build/motion/selinux/uke_control_motion.cil" %{buildroot}/usr/share/selinux/packages/uke_control_motion.cil
install -Dm644 "$motion/packaging/uke-control-motion.service" %{buildroot}/usr/lib/systemd/system/uke-control-motion.service
install -Dm644 "$motion/packaging/org.senemos.Uke.Motion1.service" %{buildroot}/usr/share/dbus-1/system-services/org.senemos.Uke.Motion1.service
install -Dm644 "$motion/packaging/org.senemos.Uke.Motion1.conf" %{buildroot}/usr/share/dbus-1/system.d/org.senemos.Uke.Motion1.conf
install -Dm644 "$motion/packaging/org.senemos.uke.motion.policy" %{buildroot}/usr/share/polkit-1/actions/org.senemos.uke.motion.policy
install -Dm644 "$motion/docs/MOTION1.md" %{buildroot}%{_docdir}/uke-control/MOTION1.md
`);
change('\n%check\n', `
%check
(
%set_build_flags
cd %{_builddir}/uke-control-motion/uke-control
make -f packaging/motion.mk CXXFLAGS="$CXXFLAGS -std=c++20 -Wall -Wextra -Werror" LDFLAGS="$LDFLAGS" check
make -f packaging/motion.mk CXX=clang++ motion_out=build/motion-clang CXXFLAGS="-O1 -g -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all" LDFLAGS="-fsanitize=address,undefined" check
UKE_POLICY_OFFLINE_FIXTURE=1 bash tests/motion-policy.sh %{SOURCE65} build/motion/selinux/uke_control_motion.cil packaging/selinux/motion-policy.sh build/motion/policy-result.txt
bash packaging/check-motion-integration.sh "$PWD" %{buildroot} %{_specdir}/uke-control.spec
bash tests/motion-lifecycle.sh %{_specdir}/uke-control.spec
)
`);
const lifecycle = fs.readFileSync(owner+'/packaging/motion-lifecycle.sh', 'utf8').replaceAll('%', '%%');
const phase = (name, label) => `(\n# UKE_CONTROL_MOTION_${label}_BEGIN\nphase=${name}\n${lifecycle}# UKE_CONTROL_MOTION_${label}_END\n) || exit $?\n`;
// Append to outgoing sections, preserving their entire order and content.
change('\n%preun -n uke-control\n', '\n'+phase('save', 'SAVE')+'\n%preun -n uke-control\n');
change('%preun -n uke-control\n', '%preun -n uke-control\n'+phase('save-upgrade', 'PREUN_SAVE'));
change('\n%posttrans -n uke-control\n', `
# UKE_CONTROL_MOTION_POLICY_INSTALL_BEGIN
/usr/libexec/uke-control/motion-policy install || exit $?
if /usr/bin/selinuxenabled;then /usr/bin/restorecon /usr/libexec/uke-control/motion || exit $?;fi
# UKE_CONTROL_MOTION_POLICY_INSTALL_END

%posttrans -n uke-control
`);
change('\n%postun -n uke-control\n', '\n'+phase('restore', 'RESTORE')+'\n%postun -n uke-control\n');
const policy = fs.readFileSync(owner+'/packaging/selinux/motion-policy.sh', 'utf8').replaceAll('%', '%%');
change('%postun -n uke-control\n', `%postun -n uke-control
# An erase/downgrade must not depend on the outgoing helper still being installed.
# UKE_CONTROL_MOTION_POLICY_ERASE_BEGIN
if [ ! -e /usr/share/selinux/packages/uke_control_motion.cil ];then
/usr/bin/bash -s -- remove <<'UKE_MOTION_POLICY_ERASE'
${policy}UKE_MOTION_POLICY_ERASE
motion_policy_status=$?
[ "$motion_policy_status" = 0 ] || exit "$motion_policy_status"
fi
# UKE_CONTROL_MOTION_POLICY_ERASE_END
${phase('restore-upgrade', 'POSTUN_RESTORE')}`);
change('\n%files\n', `
%files
/usr/libexec/uke-control/motion
/usr/libexec/uke-control/motion-policy
/usr/share/selinux/packages/uke_control_motion.cil
/usr/lib/systemd/system/uke-control-motion.service
/usr/share/dbus-1/system-services/org.senemos.Uke.Motion1.service
/usr/share/dbus-1/system.d/org.senemos.Uke.Motion1.conf
/usr/share/polkit-1/actions/org.senemos.uke.motion.policy
%doc %{_docdir}/uke-control/MOTION1.md
`);
let inverse = body;
for (const [old, value] of applied.toReversed()) {
    assert.equal(inverse.split(value).length, 2, 'Ambiguous Motion recipe inverse');
    inverse = inverse.replace(value, () => old);
}
inverse = inverse.replaceAll(`Release: ${release}%{?dist}`, 'Release: 16%{?dist}').replaceAll(`2.17.0-${release}`, '2.17.0-16');
assert.equal(inverse+history, before, 'Unrelated Control16 recipe changed');
const entry='* Sat Nov 14 2026 Uke contributors - 2.17.0-19\n- Add the demand-leased Motion1 interface within Control; keep production admission closed.\n- Preserve original sensor policy, private ownership and package rollback behavior.\n- Retain the complete Control16 source recipe and closed-cover health fix.\n\n';
const after = body+history.replace('%changelog\n', '%changelog\n'+entry);
if (contract.result?.spec_sha256) assert.equal(digest(after), contract.result.spec_sha256);
fs.writeFileSync(path, after);
console.log('Control19: complete Control16 recipe inverse passes; '+applied.length+' declared additive integration blocks; SHA256 '+digest(after));
