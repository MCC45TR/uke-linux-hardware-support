Name:           uke-cutefish-meta
Version:        1.0.0
Release:        38%{?dist}
Summary:        Independent Uke selection for the split Cutefish desktop
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-metas
Source0:        cutefish.json
Source1:        LICENSE
BuildArch:      noarch
Requires:       cutefish-desktop >= 1.0^20261115
Requires:       uke-cutefish-firstboot >= 1.0.0-2
Requires:       uke-cutefish-session-config >= 1.0.0-1

%description
Select the independently built upstream Cutefish desktop and Uke image setup
policy. The metadata RPM builds without KDE libraries. Installation does not
enable a display manager, create a human account or change hardware settings.

%prep
cp %{SOURCE1} LICENSE
%build
%install
install -Dm644 %{SOURCE0} %{buildroot}%{_datadir}/senemos/uke/desktops/cutefish.json
%files
%license LICENSE
%{_datadir}/senemos/uke/desktops/cutefish.json

%changelog
* Sun Nov 15 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-38
- Qualify the standalone Cutefish selection independently of the KDE buildroot.
