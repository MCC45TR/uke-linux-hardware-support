%global debug_package %{nil}
Name: uke-battery-observer
Version: 0.1.0
Release: 1%{?dist}
Summary: Read-only native battery observation for Xiaomi Uke
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-platform-runtime
Source0: %{name}-%{version}.tar.xz
Source1: uke-battery.conf
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros jq dbus-daemon systemd
BuildRequires: pkgconfig(libsystemd) >= 257
Requires: senemos-native-runtime(libstdc++) systemd-libs
Requires(pre): systemd

%description
C++ standard power-supply collector and read-only D-Bus diagnostics. Firmware
controls charging. No raw-property writes, PD negotiation, thresholds, factory
access or calibration changes are supplied. Health and estimation algorithms
remain disabled until pack/firmware evidence is qualified. This source
candidate has not been built, tested or physically accepted.

%prep
%setup -q
sha256sum -c SOURCE-SHA256SUMS

%build
%make_build CXXFLAGS="%{optflags}" LDFLAGS="%{build_ldflags}"

%install
%make_install prefix=%{_prefix}

%check
# Host software checks; never tablet acceptance or automatic device access.
%make_build test-native

%pre
%sysusers_create_compat %{SOURCE1}

%post
%systemd_post uke-battery.service

%preun
%systemd_preun uke-battery.service

%postun
%systemd_postun uke-battery.service

%files
%license LICENSE
%doc README.md docs/BATTERY.md docs/RUNTIME-PROFILE.md SOURCE-SHA256SUMS
%{_bindir}/uke-battery
%{_unitdir}/uke-battery.service
%{_presetdir}/90-uke-battery.preset
%{_datadir}/dbus-1/system.d/org.senemos.Uke.Battery1.conf
%{_datadir}/dbus-1/system-services/org.senemos.Uke.Battery1.service
%{_sysusersdir}/uke-battery.conf
%{_datadir}/uke-battery/battery-profile.json
%{_datadir}/uke-battery/model-profile.json
