Name:           uke-cutefish-meta
Version:        1.0.0
Release:        40%{?dist}
Summary:        Independent Uke selection for the split Cutefish desktop
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-metas
Source0:        cutefish.json
Source1:        LICENSE
BuildArch:      noarch
Requires:       cutefish-desktop >= 1.0^20261115-2
Requires:       uke-cutefish-firstboot >= 1.0.0-2
Requires:       uke-cutefish-session-config >= 1.0.0-2

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
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-40
- Declare the unified Uke Unstable repository for the Rawhide Cutefish selection.

* Sun Nov 15 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-39
- Require the corrected single-owner session data and complete portal selection.

* Sun Nov 15 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-38
- Qualify the standalone Cutefish selection independently of the KDE buildroot.
