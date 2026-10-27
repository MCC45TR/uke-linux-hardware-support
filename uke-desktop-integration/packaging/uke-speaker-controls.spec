Name:           uke-speaker-controls
Version:        0.1.0
Release:        2%{?dist}
Summary:        KDE access to the four Uke amplifier controls
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
BuildArch:      noarch
Requires:       kmix
Requires:       senemos-uke-linux-kernel-mainline >= 7.3.0-31
BuildRequires:  desktop-file-utils
BuildRequires:  libxml2

%description
Launch the original Fedora KDE KMix with its ALSA backend to adjust the four
Uke speaker amplifier levels. Playback remains stereo with automatic display
orientation. No replacement KDE application or independent surround PCM is added.

%prep
%autosetup

%build

%install
install -Dm0644 src/uke-speaker-controls.desktop %{buildroot}%{_datadir}/applications/org.uke.SpeakerControls.desktop
install -Dm0644 src/kmix/ALSA.POCO-Pad-X1.1.default.xml %{buildroot}%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.SpeakerControls.desktop
xmllint --nonet --noout %{buildroot}%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml

%files
%license LICENSE
%doc SPEAKER-CONTROLS.md
%{_datadir}/applications/org.uke.SpeakerControls.desktop
%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml

%changelog
* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Default to four amplifier level sliders using the original KMix profile format.

* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Expose the existing four amplifier controls through the original Fedora KMix.
