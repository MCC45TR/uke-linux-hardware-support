%global debug_package %{nil}
Name:           uke-video-session
Version:        1.0.0
Release:        2%{?dist}
Summary:        Scoped scheduling and power hints for original Chrome video
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
ExclusiveArch:  aarch64
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  desktop-file-utils
Requires:       google-chrome-stable
Requires:       uke-vaapi-v4l2 >= 1.0.0-8
Requires:       /usr/libexec/power-profiles-daemon

%description
Launch the original Chrome Default profile with the existing Iris CPU-copy
mode. Apply capacity-based CPU hints only to its descendant GPU processes
and hold the standard performance profile for that browser session. The
power daemon releases this client's hold on exit or manual profile changes.
No browser build, frame interpolation or clock/thermal override is included.

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

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-video-session
%{_datadir}/applications/org.uke.VideoSession.desktop

%changelog
* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Keep a single standalone runtime package without debug subpackages.

* Fri Oct 30 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Scope original-profile Chrome CPU-copy, GPU scheduling and power hints.
- Keep renderer affinity, user profile choices and hardware limits intact.
