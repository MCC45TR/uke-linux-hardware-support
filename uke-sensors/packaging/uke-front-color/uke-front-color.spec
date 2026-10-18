Name: uke-front-color
Version: 0.1.0
Release: 1%{?dist}
Summary: Native raw front-color snapshot publisher for Uke
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros
Requires: uke-ssc-fileservice >= 2026.09.21-8
%{?systemd_requires}

%description
An explicitly activated native SSC client publishes bounded raw front-color
snapshots for local readers. Units, calibration and DSP clock mapping remain
unverified. Original SensorProxy continues to provide ambient light. No Python,
factory data, firmware or per-device identifiers are included.

%prep
%setup -q

%build
%set_build_flags
%make_build build/uke-front-color

%install
install -Dm755 build/uke-front-color %{buildroot}%{_libexecdir}/uke-front-color
install -Dm644 packaging/systemd/uke-front-color.service %{buildroot}%{_unitdir}/uke-front-color.service
install -Dm644 packaging/systemd/90-uke-front-color.preset %{buildroot}%{_presetdir}/90-uke-front-color.preset

%check
%make_build test-front-color

%post
%systemd_post uke-front-color.service
%preun
%systemd_preun uke-front-color.service
%postun
%systemd_postun uke-front-color.service

%files
%license LICENSE
%doc docs/FRONT-COLOR.md
%{_libexecdir}/uke-front-color
%{_unitdir}/uke-front-color.service
%{_presetdir}/90-uke-front-color.preset

%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add an opt-in raw front-color endpoint with explicit freshness and invalidation.
