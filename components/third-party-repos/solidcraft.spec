%global debug_package %{nil}
%global _build_id_links none
%global __brp_strip %{nil}
%global __brp_strip_comment_note %{nil}

Name:           solidcraft
Version:        0.45.0
Release:        1.uke%{?dist}
Summary:        Parametric 3D CAD application for Linux AArch64
License:        MIT OR Apache-2.0
URL:            https://solidcraft.io/
Source0:        https://github.com/tangency-labs/solidcraft/releases/download/v%{version}/solidcraft-%{version}-linux-aarch64.rpm
ExclusiveArch:  aarch64
BuildRequires:  cpio
BuildRequires:  rpm
AutoReqProv:    no
Requires:       libxkbcommon.so.0()(64bit)
Requires:       libxkbcommon-x11.so.0()(64bit)
Requires:       libwayland-client.so.0()(64bit)
Requires:       libX11.so.6()(64bit)
Requires:       libX11-xcb.so.1()(64bit)
Requires:       libxcb.so.1()(64bit)
Requires:       libXcursor.so.1()(64bit)
Requires:       libXi.so.6()(64bit)
Requires:       (libvulkan.so.1()(64bit) or libEGL.so.1()(64bit))

%description
SolidCraft is a parametric 3D CAD application. This RPM contains the AArch64
desktop and command-line programs from the upstream release, signed by COPR.

%prep
printf '%s  %s\n' 6573041f777e973877b0eb01c0b917e2278e36887a50905d49cf5cc685d7f751 "%{SOURCE0}" | sha256sum -c --strict
test "$(rpm -qp --qf '%%{NAME} %%{VERSION} %%{ARCH}' %{SOURCE0})" = "%{name} %{version} aarch64"

%build

%install
mkdir -p %{buildroot}
set -o pipefail
(
    cd %{buildroot}
    rpm2cpio %{SOURCE0} | cpio -idmu --quiet --no-absolute-filenames
)

%check
test -x %{buildroot}%{_bindir}/solidcraft
test -x %{buildroot}%{_bindir}/solidcraft-cli
test -s %{buildroot}%{_datadir}/applications/io.tangencylabs.solidcraft.desktop
test -s %{buildroot}%{_docdir}/solidcraft/THIRD-PARTY-LICENSES.txt

%post
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q %{_datadir}/applications || :
fi
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database %{_datadir}/mime || :
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f %{_datadir}/icons/hicolor || :
fi
exit 0

%postun
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q %{_datadir}/applications || :
fi
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database %{_datadir}/mime || :
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f %{_datadir}/icons/hicolor || :
fi
exit 0

%files
%{_bindir}/solidcraft
%{_bindir}/solidcraft-cli
%{_datadir}/applications/io.tangencylabs.solidcraft.desktop
%{_datadir}/icons/hicolor/*/apps/io.tangencylabs.solidcraft.png
%{_datadir}/metainfo/io.tangencylabs.solidcraft.metainfo.xml
%{_datadir}/mime/packages/io.tangencylabs.solidcraft.xml
%dir %{_docdir}/solidcraft
%license %{_docdir}/solidcraft/LICENSE-APACHE
%license %{_docdir}/solidcraft/LICENSE-MIT
%license %{_docdir}/solidcraft/NOTICE
%doc %{_docdir}/solidcraft/README.md
%license %{_docdir}/solidcraft/THIRD-PARTY-LICENSES.txt

%changelog
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.45.0-1.uke
- Package the pinned original AArch64 release payload for signed COPR delivery.
