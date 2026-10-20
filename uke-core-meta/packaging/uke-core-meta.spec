%global debug_package %{nil}
Name: uke-core-meta
Version: 1.0.1
Release: 1%{?dist}
Summary: Console package selection for the Senemos Uke build candidate
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-core-meta
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz
Requires: (senemos-uke-linux-kernel-mainline >= 7.2.11-16 or senemos-uke-linux-kernel-mainline-singleboot >= 7.2.11-16)
Requires: uke-core >= 1.0.21 uke-hw >= 1.1.1 uke-power >= 1.0.15 uke-sensors
Requires: uke-control uke-kernel-update uke-installation-profile
Requires: uke-touchscreen uke-bluetooth uke-wifi-factory
Requires: uke-adaptive-display uke-power-profile-backend uke-battery-observer
Requires: uke-ssc-fileservice uke-upower
Requires: bash coreutils util-linux-core kmod
Requires: NetworkManager openssh-server sudo
Requires: senemos-native-runtime(libstdc++)
%description
A minimal console package selection and explicit Uke readiness record.
No unqualified firmware, Nabu hardware services or boot selection is installed.
Package installation does not establish a working Uke console or tablet boot.
%prep
%setup -q
%build
%install
install -Dm644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/core/profile.json
%files
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/core/profile.json

%changelog
* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.1-1
- Require device runtime packages without changing the installed boot profile.
- Allow the unmodified Fedora desktop packages and their original dependencies.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Require the native GNU C++ runtime without inherited Python debugger helpers.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Enforce the tablet's no-Python runtime policy during dependency solving.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
