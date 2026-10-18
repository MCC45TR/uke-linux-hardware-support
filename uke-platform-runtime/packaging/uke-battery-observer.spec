%global debug_package %{nil}
Name: uke-battery-observer
Version: 0.1.0
Release: 4%{?dist}
Summary: Read-only native battery observation for Xiaomi Uke
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-platform-runtime
Source0: %{name}-%{version}.tar.xz
Source1: uke-battery.conf
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros jq dbus-daemon systemd
BuildRequires: pkgconfig(libsystemd) >= 257
BuildRequires: pkgconfig(libcrypto)
Requires: senemos-native-runtime(libstdc++) systemd-libs
Requires(pre): systemd

%description
C++ standard power-supply collector and read-only D-Bus diagnostics. Firmware
controls charging. No raw-property writes, PD negotiation, thresholds, factory
access or calibration changes are supplied. Health and estimation algorithms
remain disabled until pack/firmware evidence is qualified. This source
candidate has not been built, tested or physically accepted.

%package -n uke-charge-control
Summary: Evidence-gated native Uke charge-control backend
Requires: senemos-native-runtime(libstdc++) systemd-libs
Requires(pre): systemd

%description -n uke-charge-control
Separate privileged threshold/current helper and disabled admission services. Controls
require the reviewed kernel option, exact per-boot evidence, numerical guards
and matching firmware setting readback. Positive FCC writes use the separately
admitted OEM property. Voltage and PD remain firmware-owned; no voltage writer.
The shipped profile is unqualified. No physical acceptance is implied.

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

%post -n uke-charge-control
%systemd_post uke-charge-control.service uke-charge-automatic.service

%preun -n uke-charge-control
%systemd_preun uke-charge-control.service uke-charge-automatic.service

%postun -n uke-charge-control
%systemd_postun uke-charge-control.service uke-charge-automatic.service

%files -n uke-charge-control
%license LICENSE
%doc docs/CHARGE-CONTROL.md
%{_libexecdir}/uke-charge-control
%{_unitdir}/uke-charge-control.service
%{_unitdir}/uke-charge-automatic.service
%{_presetdir}/91-uke-charge-control.preset
%config(noreplace) %{_sysconfdir}/uke-battery/control-profile.json

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

%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Add an explicit matched-kernel query for firmware-reported charge thresholds.
- Preserve read-only status and control qualification; reject stale or mixed-epoch readback.
