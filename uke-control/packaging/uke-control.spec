Name: uke-control
Version: 0.1.0
Release: 10%{?dist}
Summary: Native Uke settings backend and command-line client
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64 x86_64
BuildRequires: gcc-c++ make pkgconfig(libdrm) pkgconfig(libsystemd)
BuildRequires: pkgconfig(wayland-client) wayland-devel wayland-protocols-devel
BuildRequires: pkgconfig(libselinux) systemd-rpm-macros
Requires: util-linux-core util-linux systemd
%{?systemd_requires}

%description
C++ backend and terminal client for Uke device settings. A systemd service
records the partition layout and prefers an existing linux_swap partition;
otherwise it creates a swap file using half RAM by default. Existing partitions
are never formatted. Charge and USB controls follow real driver capabilities.

%package tests
Summary: Development-only Uke display, touch and sensor diagnostics
Requires: %{name}%{?_isa} = %{version}-%{release}

%description tests
Development diagnostics for DPU refresh measurement, Wayland presentation,
continuous touch, SensorProxy and SSC sensors. Excluded from release image
package selections. Numeric readings alone do not establish physical acceptance.

%prep
%setup -q

%build
%set_build_flags
%make_build

%install
install -Dm755 build/uke-control %{buildroot}%{_bindir}/uke-control
install -Dm755 build/libuke-control.so.0 %{buildroot}%{_libdir}/libuke-control.so.0
install -Dm644 src/backend.hpp %{buildroot}%{_includedir}/uke-control/backend.hpp
install -Dm644 src/charge-policy.hpp %{buildroot}%{_includedir}/uke-control/charge-policy.hpp
install -Dm644 src/front-color.hpp %{buildroot}%{_includedir}/uke-control/front-color.hpp
install -Dm755 build/uke-control-tests %{buildroot}%{_libexecdir}/uke-control/tests
install -Dm755 build/ssc-discovery %{buildroot}%{_libexecdir}/uke-control/ssc-discovery
install -Dm755 build/ssc-sample %{buildroot}%{_libexecdir}/uke-control/ssc-sample
install -Dm644 packaging/uke-control.service %{buildroot}%{_unitdir}/uke-control.service
install -Dm644 packaging/80-uke-control.preset %{buildroot}%{_presetdir}/80-uke-control.preset
install -Dm644 packaging/uke-control-tests.desktop %{buildroot}%{_datadir}/applications/uke-control-tests.desktop
install -d -m700 %{buildroot}%{_localstatedir}/lib/uke-control

%check
%make_build check

%post
%systemd_post uke-control.service
%preun
%systemd_preun uke-control.service
%postun
%systemd_postun uke-control.service

%files
%license LICENSE
%doc README.md CHARGING.md DISPLAY.md
%{_bindir}/uke-control
%{_libdir}/libuke-control.so.0
%{_includedir}/uke-control/*.hpp
%{_unitdir}/uke-control.service
%{_presetdir}/80-uke-control.preset
%dir %attr(0700,root,root) %{_localstatedir}/lib/uke-control

%files tests
%{_libexecdir}/uke-control/
%{_datadir}/applications/uke-control-tests.desktop

%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-10
- Add an explicit native read-only firmware threshold query.
- Keep diagnostic GETs separate from metadata status and qualified control requests.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-9
- Report native charging metadata without probing guarded threshold attributes.
- Preserve helper failures and mark unavailable generic threshold reads explicitly.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-8
- Select the active internal DSI CRTC when external display resources also exist.
- Use its resource index for vblank measurements and reject ambiguous selection.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-7
- Query capability names and verify their provided ABI versions explicitly.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Read the optional native front-color snapshot with independent freshness checks.
- Invoke the one-shot manual CDC API without a persistent service or saved opt-in.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Connect qualified charge requests and metadata to the native backend
- Add descriptor-bound front color-sensor diagnostics to development tests

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Add cached USB topology, bounded observation, guarded Type-C requests and explicit CDC opt-in.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Integrate read-only charging policy previews with explicit unavailable actuator state.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Preserve distribution linker hardening and include the actual UTC report timestamp.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add a shared C++ backend, persistent swap policy and separate development diagnostics.
