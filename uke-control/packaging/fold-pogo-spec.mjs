// SPDX-License-Identifier: MIT
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [path, manifest, owner] = process.argv.slice(2);
const contract = JSON.parse(fs.readFileSync(manifest));
const before = fs.readFileSync(path, 'utf8');
assert.equal(crypto.createHash('sha256').update(before).digest('hex'), contract.parent.spec_sha256);
assert.equal(contract.result.version, '2.17.0-14');
const split = before.indexOf('%changelog\n');
assert(split > 0);
let body = before.slice(0, split);
const history = before.slice(split), applied = [];
const change = (old, value) => {
    assert.equal(body.split(old).length, 2, 'Ambiguous pogo recipe delta');
    body = body.replace(old, () => value);
    applied.push([old, value]);
};
assert.equal((body.match(/Release: 12%\{\?dist\}/g) || []).length, 3);
body = body.replaceAll('Release: 12%{?dist}', 'Release: 14%{?dist}').replaceAll('2.17.0-12', '2.17.0-14');
const license = 'MIT AND Apache-2.0 AND GPL-3.0-or-later AND GPL-2.0-only AND (GPL-2.0-only WITH Linux-syscall-note)';
change('\nSourceLicense: '+license+'\n', '\nSourceLicense: '+license+' AND BSD-2-Clause AND BSD-3-Clause\n');
change('\nLicense: '+license+'\n', '\nLicense: '+license+' AND BSD-2-Clause AND BSD-3-Clause\n');
change('Source60: uke-control-fastrpc-provider.tar.xz\n', `Source60: uke-control-fastrpc-provider.tar.xz
Source61: uke-control-pogo-integration.tar.xz
BuildRequires: pkgconfig(Qt6Core) pkgconfig(Qt6DBus) openssl-devel patch pkgconfig(qcomtee) pkgconfig(libcbor)
Requires(pre): systemd coreutils rpm
Requires(posttrans): systemd coreutils
Requires: uke-cdsp-pogo-firmware >= 0.2-9
Provides: uke-pogo-auth = 0.1-5%{?dist}
Provides: uke-pogo-auth%{?_isa} = 0.1-5%{?dist}
Provides: uke-pogo-desktop = 0.1-6%{?dist}
Provides: uke-pogo-desktop%{?_isa} = 0.1-6%{?dist}
Provides: uke-control(pogo-integration-v1) = 1
Provides: bundled(minkipc) = 1.0.0
Obsoletes: uke-pogo-auth < 0.1-5%{?dist}
Obsoletes: uke-pogo-desktop < 0.1-6%{?dist}
`);
const oldFilter = '%global __requires_exclude_from ^%{_libdir}/qt6/plugins/ksystemstats/libuke_adreno[.]so$\n';
const optional = contract.optional_desktop_requires.map(cap => `Requires: (${cap} if powerdevil)`).join('\n');
change(oldFilter, '%global __requires_exclude_from ^(%{_libdir}/qt6/plugins/ksystemstats/libuke_adreno[.]so|%{_libexecdir}/uke-pogo-desktop)$\n'+optional+'\n');
change('\n%prep\n', `
%prep
pogo_overlay=%{_builddir}/uke-control-pogo-integration
mkdir "$pogo_overlay"
tar -xJf %{SOURCE61} -C "$pogo_overlay"
(cd "$pogo_overlay";sha256sum -c --strict --quiet SHA256SUMS)
mkdir -p "$pogo_overlay/auth/upstream/minkipc" "$pogo_overlay/auth/upstream/quic-teec" "$pogo_overlay/auth/upstream/qcbor"
for library in minkipc quic-teec qcbor;do
 tar -xzf "$pogo_overlay/upstreams/$library.tar.gz" --strip-components=1 -C "$pogo_overlay/auth/upstream/$library"
done
(cd "$pogo_overlay/auth";patch -p1 --fuzz=0 < uke-pogo-auth/patches/0001-qcomtee-refuse-credential-encoding-errors.patch)
`);
change('\n%build\n', `
%build
(
%set_build_flags
pogo_overlay=%{_builddir}/uke-control-pogo-integration
cd "$pogo_overlay/auth"
bash ../packaging/build-pogo-auth.sh
cd "$pogo_overlay/desktop"
bash packaging/build.sh "$PWD/build"
)
`);
change('\n%install\n', `
%install
pogo_overlay=%{_builddir}/uke-control-pogo-integration
install -Dm755 "$pogo_overlay/auth/build/uke-pogo-auth" %{buildroot}/usr/libexec/uke-pogo-auth
install -Dm755 "$pogo_overlay/desktop/build/uke-pogo-desktop" %{buildroot}/usr/libexec/uke-pogo-desktop
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/packaging/uke-pogo-auth.service" %{buildroot}/usr/lib/systemd/system/uke-pogo-auth.service
install -Dm644 "$pogo_overlay/desktop/packaging/pogo-attachment.conf" %{buildroot}/usr/lib/systemd/system/uke-pogo-auth.service.d/pogo-attachment.conf
install -Dm644 "$pogo_overlay/desktop/packaging/uke-pogo-desktop.service" %{buildroot}/usr/lib/systemd/user/uke-pogo-desktop.service
install -Dm644 "$pogo_overlay/desktop/packaging/70-uke-pogo.rules" %{buildroot}/usr/lib/udev/rules.d/70-uke-pogo.rules
install -Dm644 "$pogo_overlay/desktop/packaging/uke-pogo-desktop.desktop" %{buildroot}/etc/xdg/autostart/uke-pogo-desktop.desktop
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/MIT-LICENSE.txt
for library in minkipc quic-teec;do
 install -Dm644 "$pogo_overlay/auth/upstream/$library/LICENSE.txt" "%{buildroot}%{_licensedir}/%{name}/pogo/$library-LICENSE.txt"
done
install -Dm644 "$pogo_overlay/auth/upstream/qcbor/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/qcbor-LICENSE.txt
install -Dm644 "$pogo_overlay/auth/upstream/quic-teec/libqcomtee/src/linux/tee.h" %{buildroot}%{_licensedir}/%{name}/pogo/LINARO-tee-interface-notice.h
install -Dm644 "$pogo_overlay/desktop/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/DESKTOP-LICENSE.txt
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/README.md" %{buildroot}%{_docdir}/%{name}/POGO-AUTH.md
install -Dm644 "$pogo_overlay/desktop/README.md" %{buildroot}%{_docdir}/%{name}/POGO-DESKTOP.md
install -Dm644 "$pogo_overlay/docs/POGO-INTEGRATION.md" %{buildroot}%{_docdir}/%{name}/POGO-INTEGRATION.md
`);
change('\n%check\n', `
%check
bash %{_builddir}/uke-control-pogo-integration/packaging/check-pogo-integration.sh %{_builddir}/uke-control-pogo-integration %{buildroot}
`);
const fragment = fs.readFileSync(owner+'/packaging/pogo-ownership.sh', 'utf8').replaceAll('%', '%%');
for (const [section, phase] of [['%pre -n uke-control\n', 'save'], ['%posttrans -n uke-control\n', 'restore']]) {
    const extra = `(\n# UKE_CONTROL_POGO_${phase.toUpperCase()}_BEGIN\nphase=${phase}\n${fragment}# UKE_CONTROL_POGO_${phase.toUpperCase()}_END\n) || exit $?\n`;
    change(section, section+extra);
}
change('%post -n uke-control\n', '%post -n uke-control\n%systemd_post uke-pogo-auth.service\n%systemd_user_post uke-pogo-desktop.service\n%udev_rules_update\n');
const rollback = (phase, label) => `(\n# UKE_CONTROL_POGO_${label}_BEGIN\nphase=${phase}\n${fragment}# UKE_CONTROL_POGO_${label}_END\n) || exit $?\n`;
change('%postun -n uke-control\n', '%postun -n uke-control\n%systemd_postun uke-pogo-auth.service\n%systemd_user_postun uke-pogo-desktop.service\n%udev_rules_update\n'+rollback('restore-upgrade', 'POSTUN_RESTORE'));
change('%preun -n uke-control\n', '%preun -n uke-control\n'+rollback('save-upgrade', 'PREUN_SAVE')+'%systemd_preun uke-pogo-auth.service\n%systemd_user_preun uke-pogo-desktop.service\n');
change('\n%files\n', `
%files
/usr/libexec/uke-pogo-auth
/usr/libexec/uke-pogo-desktop
/usr/lib/systemd/system/uke-pogo-auth.service
/usr/lib/systemd/system/uke-pogo-auth.service.d/pogo-attachment.conf
/usr/lib/systemd/user/uke-pogo-desktop.service
/usr/lib/udev/rules.d/70-uke-pogo.rules
/etc/xdg/autostart/uke-pogo-desktop.desktop
%license %{_licensedir}/%{name}/pogo
%doc %{_docdir}/%{name}/POGO-AUTH.md
%doc %{_docdir}/%{name}/POGO-DESKTOP.md
%doc %{_docdir}/%{name}/POGO-INTEGRATION.md
`);
let inverse = body;
for (const [old, value] of applied.toReversed()) {
    assert.equal(inverse.split(value).length, 2, 'Ambiguous pogo recipe inverse: '+old.slice(0, 48));
    inverse = inverse.replace(value, () => old);
}
inverse = inverse.replaceAll('Release: 14%{?dist}', 'Release: 12%{?dist}').replaceAll('2.17.0-14', '2.17.0-12');
assert.equal(inverse+history, before, 'Unrelated Control12 recipe changed');
const entry = '* Thu Nov 12 2026 Uke contributors - 2.17.0-14\n- Fold pogo authentication and Plasma integration into existing Control.\n- Admit the qualified 7.3.3 bridge family and block on request or stop events.\n- Preserve trusted input checks, original desktops and service preferences.\n\n';
fs.writeFileSync(path, body+history.replace('%changelog\n', '%changelog\n'+entry));
console.log('Control14: whole Control12 recipe inverse passes; only declared pogo ownership/runtime/conditional dependencies change.');
