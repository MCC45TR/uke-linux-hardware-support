Name: uke-pogo-desktop
Version: 0.1
Release: 1%{?dist}
Summary: Detect Uke pogo keyboard backlight for the original Plasma power service
License: MIT
Source0: %{name}-%{version}.tar.gz
BuildRequires: gcc-c++ clang compiler-rt
BuildRequires: pkgconfig(Qt6Core) pkgconfig(Qt6DBus)
BuildRequires: systemd-rpm-macros
Requires: powerdevil >= 6
Requires: /usr/bin/systemctl
Requires: /usr/libexec/upowerd
Requires: uke-pogo-auth >= 0.1-3
ExclusiveArch: aarch64
%description
Native unprivileged Uke helper for the standard kernel keyboard LED and UPower
interfaces. Re-detects the original PowerDevil service after a real pogo plug
change. Contains no KDE/GNOME fork, firmware or raw hardware access.
%prep
%setup -q
%build
%set_build_flags
bash packaging/build.sh "$PWD/build"
%check
bash tests/check.sh
%install
install -Dm755 build/uke-pogo-desktop %{buildroot}%{_libexecdir}/uke-pogo-desktop
install -Dm644 packaging/uke-pogo-desktop.service %{buildroot}%{_userunitdir}/uke-pogo-desktop.service
install -Dm644 packaging/uke-pogo-desktop.desktop %{buildroot}%{_sysconfdir}/xdg/autostart/uke-pogo-desktop.desktop
install -Dm644 packaging/70-uke-pogo.rules %{buildroot}%{_udevrulesdir}/70-uke-pogo.rules
install -Dm644 packaging/pogo-attachment.conf %{buildroot}%{_unitdir}/uke-pogo-auth.service.d/pogo-attachment.conf
%post
%systemd_user_post uke-pogo-desktop.service
%udev_rules_update
%preun
%systemd_user_preun uke-pogo-desktop.service
%postun
%systemd_user_postun uke-pogo-desktop.service
%udev_rules_update
%files
%license LICENSE
%{_libexecdir}/uke-pogo-desktop
%{_userunitdir}/uke-pogo-desktop.service
%{_sysconfdir}/xdg/autostart/uke-pogo-desktop.desktop
%{_udevrulesdir}/70-uke-pogo.rules
%{_unitdir}/uke-pogo-auth.service.d/pogo-attachment.conf
%doc README.md
%changelog
* Thu Nov 05 2026 Uke contributors - 0.1-1
- Re-detect the original Plasma service after actual pogo LED plug changes.
- Wait for UPower registration and preserve other keyboard backlights.
