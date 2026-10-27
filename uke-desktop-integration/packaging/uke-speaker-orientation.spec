Name:           uke-speaker-orientation
Version:        0.1.0
Release:        2%{?dist}
Summary:        Display-aware internal speaker routing for Uke Wayland sessions
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
ExclusiveArch:  aarch64
Requires:       uke-displayport-audio >= 0.1.0-6
BuildRequires:  gcc-c++ pkgconf-pkg-config alsa-lib-devel wayland-devel systemd-devel
BuildRequires:  libasan libubsan

%description
Follow the internal display transform in KDE Plasma and GNOME Wayland sessions
and set the Uke kernel's per-amplifier Speaker Rotation controls. Only the active
local session and the exact combined Uke card are eligible. No gain, speaker
enable, firmware, microphone or external-output setting is changed.

%prep
%autosetup

%build
%{__cxx} %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror \
    $(pkg-config --cflags alsa wayland-client libsystemd) \
    src/uke-speaker-orientation.cpp %{build_ldflags} \
    $(pkg-config --libs alsa wayland-client libsystemd) -o uke-speaker-orientation

%install
install -Dm0755 uke-speaker-orientation %{buildroot}%{_libexecdir}/uke-speaker-orientation
install -Dm0644 src/uke-speaker-orientation.desktop %{buildroot}%{_sysconfdir}/xdg/autostart/uke-speaker-orientation.desktop

%check
bash tests/speaker-orientation.sh

%files
%license LICENSE
%doc SPEAKER-ORIENTATION.md
%{_libexecdir}/uke-speaker-orientation
%config(noreplace) %{_sysconfdir}/xdg/autostart/uke-speaker-orientation.desktop

%changelog
* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Document the distinct release-23 kernel candidate and USB-C-right operator reference.

* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Follow committed internal-output transforms and active local session changes.
- Validate the complete kernel control ABI before updating speaker rotation.
