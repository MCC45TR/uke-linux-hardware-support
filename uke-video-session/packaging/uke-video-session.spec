%global debug_package %{nil}
Name:           uke-video-session
Version:        1.1.0
Release:        1%{?dist}
Summary:        Automatic per-user policy for native UHD video decoding
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
ExclusiveArch:  aarch64
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  systemd-rpm-macros
BuildRequires:  dbus-daemon
BuildRequires:  systemd
Requires:       systemd
Requires:       dbus
Requires:       /usr/libexec/power-profiles-daemon

%description
Activate through the user's standard D-Bus session when the native VA backend
reports UHD decode activity. Authenticate the original Chrome GPU sender, apply
reversible CPU hints and request the standard performance profile. Expire idle
activity, respect manual profile changes and restore affinity after service
failure. No browser wrapper, profile name, URL or always-running login service
is required. No frame interpolation or clock/thermal override is included.

%prep
%setup -q

%build
%set_build_flags
%make_build

%check
%set_build_flags
make check

%install
%make_install PREFIX=%{_prefix}

%post
%systemd_user_post uke-video-session.service

%preun
%systemd_user_preun uke-video-session.service

%postun
%systemd_user_postun_with_restart uke-video-session.service

%files
%license LICENSE
%doc README.md
%{_libexecdir}/uke-video-session
%{_userunitdir}/uke-video-session.service
%{_datadir}/dbus-1/services/org.uke.VideoSession1.service
%{_datadir}/dbus-1/interfaces/org.uke.VideoSession1.xml

%changelog
* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-1
- Replace the dedicated launcher with per-user D-Bus activation.
- Scope hints to fresh native UHD activity and restore idle/crashed sessions.

* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Recognize Chrome GPU titles after argv is joined for process reporting.
- Cover both NUL-separated arguments and joined titles with exact type checks.

* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Keep a single standalone runtime package without debug subpackages.

* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Scope original-profile Chrome CPU-copy, GPU scheduling and power hints.
- Keep renderer affinity, user profile choices and hardware limits intact.
