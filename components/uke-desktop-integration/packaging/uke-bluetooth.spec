Name: uke-bluetooth
Version: 0.1.0
Release: 6%{?dist}
Summary: Native factory-address integration for the Uke Bluetooth controller
License: GPL-3.0-or-later AND MIT
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ bluez-libs-devel systemd-rpm-macros
Requires: bluez systemd-udev
%{?systemd_requires}

%description
Read the six-byte device-local factory address from read-only persist and use
the standard BlueZ management interface when the exact Uke controller requests
it. Device and management events determine readiness. No calibration, unique
address or proprietary firmware is included or modified.

%prep
%autosetup

%build
%set_build_flags
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror src/bluetooth-address.cpp -o uke-bluetooth-address

%check
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-function tests/bluetooth-address.cpp -o contract
./contract

%install
install -Dm755 uke-bluetooth-address %{buildroot}%{_libexecdir}/uke-bluetooth-address
install -Dm644 packaging/uke-bluetooth-address.service %{buildroot}%{_unitdir}/uke-bluetooth-address.service
install -Dm644 packaging/70-uke-bluetooth.rules %{buildroot}%{_udevrulesdir}/70-uke-bluetooth.rules

%post
%systemd_post uke-bluetooth-address.service
%udev_rules_update

%preun
%systemd_preun uke-bluetooth-address.service

%postun
%systemd_postun uke-bluetooth-address.service
%udev_rules_update

%files
%license LICENSE LICENSE.GPL-3.0
%doc docs/BLUETOOTH.md
%{_libexecdir}/uke-bluetooth-address
%{_unitdir}/uke-bluetooth-address.service
%{_udevrulesdir}/70-uke-bluetooth.rules

%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Require physical protected persist and reject redirected factory address files.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Process Linux Command Status when an early management send returns zero.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Attach explicitly enabled factory setup to the standard Bluetooth target.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Include the native helper's full GPL license text alongside the component license.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Accept a configured controller whose configuration query reports no missing options.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Admit the exact Uke factory address after kernel controller readiness.
