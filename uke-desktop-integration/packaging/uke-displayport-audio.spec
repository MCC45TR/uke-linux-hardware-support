Name:           uke-displayport-audio
Version:        0.1.0
Release:        10%{?dist}
Summary:        Internal microphone, speaker and DisplayPort policy for Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
BuildArch:      noarch
Requires:       alsa-ucm
Requires:       wireplumber >= 0.5
Requires:       pipewire-pulseaudio >= 1.6.10-1.uke1
BuildRequires:  gcc-c++ alsa-lib-devel alsa-utils pkgconf-pkg-config

%description
ALSA Use Case Manager data for the POCO-Pad-X1 and POCO-Pad-X1-HDMI cards.
The combined card exposes four speaker PCM channels and a separate stereo
DisplayPort PCM. Four independent mono microphone sources share one four-channel
capture PCM, gated on the corrected native capture capability. The amplifier
preset is supplied separately.

%prep
%autosetup

%build

%install
install -d %{buildroot}%{_datadir}/alsa/ucm2/Qualcomm/sm7675
install -m 0644 src/ucm2/Qualcomm/sm7675/*.conf %{buildroot}%{_datadir}/alsa/ucm2/Qualcomm/sm7675/
install -d %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675
ln -s ../../Qualcomm/sm7675/POCO-Pad-X1-HDMI.conf %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf
ln -s ../../Qualcomm/sm7675/POCO-Pad-X1.conf %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1.conf
install -Dm0644 src/wireplumber/75-uke-thp-not-camera.conf %{buildroot}%{_datadir}/wireplumber/wireplumber.conf.d/75-uke-thp-not-camera.conf

%check
test "$(find %{buildroot}%{_datadir}/alsa/ucm2/Qualcomm/sm7675 -type f | wc -l)" -eq 9
test "$(find %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675 -type l | wc -l)" -eq 2
cmp src/wireplumber/75-uke-thp-not-camera.conf %{buildroot}%{_datadir}/wireplumber/wireplumber.conf.d/75-uke-thp-not-camera.conf
test -r %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf
bash tests/audio-policy.sh

%files
%license LICENSE
%dir %{_datadir}/alsa/ucm2/Qualcomm/sm7675
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/POCO-Pad-X1-HDMI.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/DP-HiFi.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/DP-ELD.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/POCO-Pad-X1.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/Speaker-*.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/Microphone-*.conf
%dir %{_datadir}/alsa/ucm2/conf.d/sm7675
%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf
%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1.conf
%{_datadir}/wireplumber/wireplumber.conf.d/75-uke-thp-not-camera.conf

%changelog
* Sat Oct 31 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-10
- Require the corrected raw microphone channel map before desktop exposure.

* Sat Oct 31 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-9
- Expose four selectable mono microphones using the shared native capture PCM.
- Require the corrected multichannel lifecycle marker and preserve separate playback devices.

* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-7
- Expose four internal playback channels while preserving the independent stereo DisplayPort route.

* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Add the combined Uke card, bounded stereo-to-four-slot routing and speaker switches.
- Preserve the original HDMI-only card and require no microphone or tuning payload.

* Sun Oct 25 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Keep the Uke memory-to-memory video codec out of camera discovery.
- Preserve direct codec access, USB camera discovery and the digital audio route.

* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Expose the native DP backend ELD to frontend discovery and monitor labels.

* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Exclude the protected THP raw interface from camera discovery to unblock media routing.

* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Enable the sole DP backend before desktop PCM probing.

* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add the digital-only DisplayPort playback policy.
