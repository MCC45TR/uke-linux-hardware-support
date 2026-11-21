%global debug_package %{nil}
%global _build_id_links none
%global __brp_strip %{nil}
%global __brp_strip_comment_note %{nil}
%global __brp_strip_lto %{nil}
%global __provides_exclude_from ^%{_datadir}/rustdesk/lib/
%global __requires_exclude ^lib(app|desktop_.*_plugin|file_selector_linux_plugin|flutter_.*|rustdesk|screen_retriever_plugin|texture_rgba_renderer_plugin|url_launcher_linux_plugin|window_.*_plugin)\.so\(\)\(64bit\)$

Name:           rustdesk
Version:        1.5.3
Release:        1.uke%{?dist}
Summary:        Remote desktop client for Linux AArch64
License:        AGPL-3.0-only
URL:            https://github.com/rustdesk/rustdesk
Source0:        %{url}/releases/download/%{version}/rustdesk-%{version}-0.aarch64.rpm
Source1:        https://raw.githubusercontent.com/rustdesk/rustdesk/95cca3f661f635b3cc85c04935fe8d96d8f432ef/LICENCE
ExclusiveArch:  aarch64
BuildRequires:  binutils
BuildRequires:  chrpath
BuildRequires:  cpio
BuildRequires:  rpm
BuildRequires:  systemd-rpm-macros
Requires:       alsa-lib
Requires:       gstreamer1-plugins-base
Requires:       gtk3
Requires:       libXfixes
Requires:       libva
Requires:       libxcb
%{?systemd_requires}

%description
RustDesk provides remote desktop access. This RPM uses the official
AArch64 release with normal systemd service presets. Corresponding source
is available from the RustDesk %{version} release.

%prep
%setup -q -c -T
printf '%%s  %%s\n' a36cb8ff424896ffd10df9e9cd8d73b6a00a70128202ac2b550169ff84dd6868 '%{SOURCE0}' | sha256sum -c --strict
printf '%%s  %%s\n' 8486a10c4393cee1c25392769ddd3b2d6c242d6ec7928e1414efff7dfb2f07ef '%{SOURCE1}' | sha256sum -c --strict
test "$(rpm -qp --qf '%%{NAME} %%{VERSION} %%{ARCH}' '%{SOURCE0}')" = '%{name} %{version} aarch64'
set -o pipefail
rpm2cpio '%{SOURCE0}' | cpio -idm --quiet --no-absolute-filenames
test -z "$(find usr -iname '*.py' -o -iname '*.pyc' -o -iname '*libpython*')"
LC_ALL=C readelf -h usr/share/rustdesk/rustdesk | grep -Eq 'Machine: *AArch64'
# Flutter plugins should resolve sibling libraries from the installed bundle.
for library in usr/share/rustdesk/lib/*_plugin.so usr/share/rustdesk/lib/librustdesk.so; do
    chrpath --replace '$ORIGIN' "$library"
done
install -m0644 '%{SOURCE1}' LICENCE

%build

%install
install -dm0755 '%{buildroot}%{_bindir}' '%{buildroot}%{_datadir}'
cp -a usr/share/rustdesk '%{buildroot}%{_datadir}/'
cp -a usr/share/icons '%{buildroot}%{_datadir}/'
ln -s ../share/rustdesk/rustdesk '%{buildroot}%{_bindir}/rustdesk'
install -Dm0644 usr/share/rustdesk/files/rustdesk.service '%{buildroot}%{_unitdir}/rustdesk.service'
install -Dm0644 usr/share/rustdesk/files/rustdesk.desktop '%{buildroot}%{_datadir}/applications/rustdesk.desktop'
install -Dm0644 usr/share/rustdesk/files/rustdesk-link.desktop '%{buildroot}%{_datadir}/applications/rustdesk-link.desktop'

%post
%systemd_post rustdesk.service

%preun
%systemd_preun rustdesk.service

%postun
%systemd_postun_with_restart rustdesk.service

%files
%license LICENCE
%{_bindir}/rustdesk
%{_unitdir}/rustdesk.service
%{_datadir}/applications/rustdesk.desktop
%{_datadir}/applications/rustdesk-link.desktop
%{_datadir}/icons/hicolor/256x256/apps/rustdesk.png
%{_datadir}/icons/hicolor/scalable/apps/rustdesk.svg
%{_datadir}/rustdesk/

%changelog
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.5.3-1.uke
- Package the upstream AArch64 payload and honor Fedora service presets.
