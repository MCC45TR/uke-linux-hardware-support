Name:           uke-displayport-audio
Version:        0.1.0
Release:        2%{?dist}
Summary:        DisplayPort playback policy for POCO Pad X1
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.gz
BuildArch:      noarch
Requires:       alsa-ucm

%description
ALSA Use Case Manager configuration for the POCO-Pad-X1-HDMI sound card.
Routes the single digital playback frontend to DisplayPort. Internal speaker
and microphone paths are not configured.

%prep
%autosetup

%build

%install
install -d %{buildroot}%{_datadir}/alsa/ucm2/Qualcomm/sm7675
install -m 0644 src/ucm2/Qualcomm/sm7675/*.conf %{buildroot}%{_datadir}/alsa/ucm2/Qualcomm/sm7675/
install -d %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675
ln -s ../../Qualcomm/sm7675/POCO-Pad-X1-HDMI.conf %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf

%check
test "$(find %{buildroot} -type f | wc -l)" -eq 2
test "$(find %{buildroot} -type l | wc -l)" -eq 1
test -r %{buildroot}%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf

%files
%license LICENSE
%dir %{_datadir}/alsa/ucm2/Qualcomm/sm7675
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/POCO-Pad-X1-HDMI.conf
%{_datadir}/alsa/ucm2/Qualcomm/sm7675/DP-HiFi.conf
%dir %{_datadir}/alsa/ucm2/conf.d/sm7675
%{_datadir}/alsa/ucm2/conf.d/sm7675/POCO-Pad-X1-HDMI.conf

%changelog
* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Enable the sole DP backend before desktop PCM probing.

* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add the digital-only DisplayPort playback policy.
