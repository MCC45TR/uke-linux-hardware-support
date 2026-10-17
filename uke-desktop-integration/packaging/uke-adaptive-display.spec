Name: uke-adaptive-display
Version: 0.1.0
Release: 6%{?dist}
Summary: Sensor integration for automatic display rotation and brightness
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
BuildArch: noarch
BuildRequires: systemd-rpm-macros systemd-udev
Requires: iio-sensor-proxy(ssc) >= 1
Requires: uke-ssc-fileservice >= 2026.09.21-8
Requires: grep systemd-udev
%{?systemd_requires}

%description
Qualcomm SSC support for the standard desktop SensorProxy interface. Original
KDE and other compatible desktop clients control rotation and ambient brightness.
This preview installs a manually enabled integration target and test instructions.
It includes no firmware, calibration, desktop replacement or brightness daemon.

%prep
%setup -q

%build

%install
install -Dm644 packaging/uke-adaptive-display.target %{buildroot}%{_unitdir}/uke-adaptive-display.target
install -Dm644 packaging/80-uke-adaptive-display.preset %{buildroot}%{_presetdir}/80-uke-adaptive-display.preset
install -Dm644 packaging/71-uke-tablet-mode.rules %{buildroot}%{_udevrulesdir}/71-uke-tablet-mode.rules

%check
udevadm verify packaging/71-uke-tablet-mode.rules

%post
%systemd_post uke-adaptive-display.target
%udev_rules_update

%preun
%systemd_preun uke-adaptive-display.target

%postun
%systemd_postun uke-adaptive-display.target
%udev_rules_update

%files
%license LICENSE
%doc docs/ADAPTIVE-DISPLAY.md
%{_unitdir}/uke-adaptive-display.target
%{_presetdir}/80-uke-adaptive-display.preset
%{_udevrulesdir}/71-uke-tablet-mode.rules

%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Keep rear-cover state and external mice from overriding tablet attachment detection.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Remove the ineffective SDDM shutdown ordering from sensor integration.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Require the dual-light backend and stop user managers before SDDM.

* Sat Oct 17 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Start an explicitly enabled target when its FastRPC device appears.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Package standard desktop sensor integration for controlled device tests.
- Keep activation explicit and preserve read-only calibration inputs.
