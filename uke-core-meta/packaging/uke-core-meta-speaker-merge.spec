%global debug_package %{nil}
Name: uke-core-meta
Version: 1.0.7
Release: 20%{?dist}
License: MIT


Summary: Console package selection for the Senemos Uke build candidate
Requires: (senemos-uke-linux-kernel-mainline >= 7.3.0-33 or senemos-uke-linux-kernel-mainline-singleboot >= 7.3.0-33)
Requires: uke-control >= 2.3.1 uke-kernel-update uke-installation-profile
Requires: bluez libinput libwacom >= 2.20.0 uke-libssc uke-iio-sensor-proxy
Requires: uke-pipewire uke-pipewire-alsa wireplumber alsa-utils
Requires: uke-displayport-audio >= 0.1.0-8 uke-speaker-orientation >= 0.1.0-5 uke-speaker-firmware >= 3.0.304.0-2
Requires: (uke-speaker-controls >= 0.1.0-4 if plasma-workspace)
Requires: uke-touchscreen
Requires: uke-boot-integration(factory-bluetooth-v1) >= 1
Requires: uke-boot-integration(factory-wlan-v1) >= 1
Requires: uke-adaptive-display uke-battery-observer
Requires: power-profiles-daemon
Requires: (uke-kernel(platform-profile-qos-v1) >= 1 or uke-power-profile-backend)
Requires: uke-ssc-fileservice uke-upower
Requires: bash coreutils util-linux-core kmod
Requires: NetworkManager openssh-server sudo
Requires: audit audit-rules strace
Requires: uke-mesa-libOpenCL
Requires: uke-mesa-libclc22-spirv
Requires: uke-vaapi-v4l2 >= 1.0.0-12.fc46
Requires: libstdc++(aarch-64) >= 16.2.1
Recommends: uke-plymonth >= 1.0.2-3

Provides: uke-core = 2.3.1-1.fc46
Obsoletes: uke-core <= 2.3.1-1.fc46
Provides: uke-hw = 1.8.5-1.fc46
Obsoletes: uke-hw <= 1.8.5-1.fc46
Provides: uke-power = 1.6.0-1.fc46
Obsoletes: uke-power <= 1.6.0-1.fc46
Provides: uke-sensors = 1.0.9-5.fc46
Obsoletes: uke-sensors <= 1.0.9-5.fc46
Provides: uke-extras = 1.0.4-2.fc46
Obsoletes: uke-extras <= 1.0.4-2.fc46
Provides: uke-additional = 1.0.0-16.fc46
Obsoletes: uke-additional <= 1.0.0-16.fc46
Provides: uke-metas = 1.4.0-4.fc46
Obsoletes: uke-metas <= 1.4.0-4.fc46
Provides: uke-camera = 1.0.0-3.fc46
Obsoletes: uke-camera <= 1.0.0-3.fc46
Provides: uke-sound = 1.0.5-1.fc46
Obsoletes: uke-sound <= 1.0.5-1.fc46
Requires: uke-control(fedora-returns-v1) >= 1
Provides: uke-core-meta(fedora-returns-v1) = 1
Provides: uke-core-meta(fedora-cxx-runtime-v1) = 1
Requires: uke-boot-integration >= 1.8.18-2.fc46 uke-battery-observer >= 1.4.1-3.fc46
Conflicts: uke-charge-control < 1.4.1-3.fc46
Conflicts: uke-offline-charge < 1.4.1-3.fc46
Conflicts: uke-atk <= 2.62.1-17.fc46
Conflicts: uke-atk-devel <= 2.62.1-17.fc46
Conflicts: uke-at-spi2-atk <= 2.62.1-17.fc46
Conflicts: uke-at-spi2-atk-devel <= 2.62.1-17.fc46
Conflicts: uke-at-spi2-core <= 2.62.1-17.fc46
Conflicts: uke-at-spi2-core-devel <= 2.62.1-17.fc46
Conflicts: uke-gstreamer1 <= 1.28.8-17.fc46
Conflicts: uke-gstreamer1-devel <= 1.28.8-17.fc46
Conflicts: uke-libwacom <= 2.20.0-18.fc46
Conflicts: uke-libwacom-data <= 2.20.0-18.fc46
Conflicts: uke-libwacom-devel <= 2.20.0-18.fc46
Conflicts: uke-libwacom-utils <= 2.20.0-18.fc46
Conflicts: uke-libstdcxx <= 16.2.1-19.fc46
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-core-meta
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: desktop-file-utils libxml2 coreutils
Provides: uke-speaker-controls = 0.1.0-6
Obsoletes: uke-speaker-controls < 0.1.0-6
Requires: (kmix if plasma-workspace)

%description
Retained Uke Core package selection with the existing four-amplifier KMix
profile and original Fedora KMix launcher. The kernel owns hardware controls;
the profile follows the desktop's standard user interface. Existing user KMix
profiles take precedence. Installation does not change audio controls or boot
selection and does not establish hardware acceptance.

%prep
%setup -q

%build

%install
install -Dm0644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/core/profile.json
install -Dm0644 speaker-controls/uke-speaker-controls.desktop %{buildroot}%{_datadir}/applications/org.uke.SpeakerControls.desktop
install -Dm0644 speaker-controls/ALSA.POCO-Pad-X1.1.default.xml %{buildroot}%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml
install -Dm0644 speaker-controls/SPEAKER-CONTROLS.md %{buildroot}%{_docdir}/uke-speaker-controls/SPEAKER-CONTROLS.md
install -Dm0644 speaker-controls/LICENSE %{buildroot}%{_licensedir}/uke-speaker-controls/LICENSE

%check
sha256sum -c --strict SOURCE-SHA256SUMS
cmp src/profile.json %{buildroot}%{_datadir}/senemos/uke/core/profile.json
cmp speaker-controls/uke-speaker-controls.desktop %{buildroot}%{_datadir}/applications/org.uke.SpeakerControls.desktop
cmp speaker-controls/ALSA.POCO-Pad-X1.1.default.xml %{buildroot}%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.SpeakerControls.desktop
xmllint --nonet --noout %{buildroot}%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml
printf 'Retained Core19.2 data and all speaker controls bytes, desktop/XML validation and script-free merge passed.\n'

%files
%license LICENSE
%doc README.md SPEAKER-CONTROLS-MERGE.md
%{_datadir}/senemos/uke/core/profile.json
%{_datadir}/applications/org.uke.SpeakerControls.desktop
%{_datadir}/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml
%dir %{_docdir}/uke-speaker-controls
%doc %{_docdir}/uke-speaker-controls/SPEAKER-CONTROLS.md
%dir %{_licensedir}/uke-speaker-controls
%license %{_licensedir}/uke-speaker-controls/LICENSE

%changelog
* Fri Nov 06 2026 Uke contributors - 1.0.7-20
- Consolidate the unchanged KMix controls profile into Core.
- Retain the complete Core19.2 selection and original Fedora KMix.
