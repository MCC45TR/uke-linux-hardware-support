%global debug_package %{nil}
Name: uke-core-meta
Version: 1.0.7
Release: 1%{?dist}
Summary: Console package selection for the Senemos Uke build candidate
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-core-meta
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz
Requires: (senemos-uke-linux-kernel-mainline >= 7.3.0-33 or senemos-uke-linux-kernel-mainline-singleboot >= 7.3.0-33)
Requires: uke-control >= 2.3.1 uke-kernel-update uke-installation-profile
Requires: bluez libinput uke-libwacom uke-libssc uke-iio-sensor-proxy
Requires: uke-pipewire uke-pipewire-alsa wireplumber alsa-utils
Requires: uke-displayport-audio >= 0.1.0-8 uke-speaker-orientation >= 0.1.0-5 uke-speaker-firmware >= 3.0.304.0-2
Requires: (uke-speaker-controls >= 0.1.0-4 if plasma-workspace)
Requires: uke-touchscreen
Requires: uke-boot-integration(factory-bluetooth-v1) >= 1
Requires: uke-boot-integration(factory-wlan-v1) >= 1
Requires: uke-adaptive-display uke-battery-observer
Requires: power-profiles-daemon
Requires: (uke-kernel(platform-profile-qos-v1) >= 1 or uke-power-profile-backend)
Requires: uke-ssc-fileservice uke-upower
Requires: bash coreutils util-linux-core kmod
Requires: NetworkManager openssh-server sudo
Requires: senemos-native-runtime(libstdc++)
Recommends: uke-plymonth >= 1.0.2-3
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
* Thu Oct 29 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.7-1
- Select individual runtime packages without source-family dependencies.

* Tue Oct 27 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.6-1
- Pull the matched speaker kernel and desktop policy through normal DNF updates.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.5-1
- Recommend the removable Uke Plymouth theme on normal DNF upgrades.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.4-1
- Select factory Bluetooth identity through the owning boot-integration capability.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.3-1
- Select factory WLAN provisioning through the owning boot-integration capability.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-1
- Accept the native kernel profile provider with the original distribution daemon.
- Retain the compatibility backend dependency for older kernel installations.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.1-1
- Require device runtime packages without changing the installed boot profile.
- Allow the unmodified Fedora desktop packages and their original dependencies.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Require the native GNU C++ runtime without inherited Python debugger helpers.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Enforce the tablet's no-Python runtime policy during dependency solving.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
