Name: uke-local-gpu-firmware
Version: 3.0.304.0
Release: 1.local%{?dist}
Summary: Device-local Adreno 732 firmware for Uke development
License: LicenseRef-Device-Local-Redistribution-Unreviewed
BuildArch: noarch
Source0: gen71100_sqe.fw
Source1: gen71100_gmu.bin
Source2: gen71100_zap.mbn

%description
Firmware retained from the tested Uke software baseline. This local package
keeps the firmware available after switching from initramfs to the root
filesystem. Redistribution permission has not been established. Do not upload
the binary package or its source RPM to a public build or download service.

%prep
echo 'b6df660d109504dbea5a266910b037a33d893a11c6de52f524a5fd112008c401  %{SOURCE0}' | sha256sum -c -
echo '435c7bac9a328b8a100df930b84b65b4d579e42be44785d0e60e3729291bb864  %{SOURCE1}' | sha256sum -c -
echo 'ac0165a5e2bb5735746d5c19ce71a964cdf99c7b36dedeb300db7496779e09b3  %{SOURCE2}' | sha256sum -c -

%build

%install
install -Dm644 %{SOURCE0} %{buildroot}/usr/lib/firmware/qcom/gen71100_sqe.fw
install -Dm644 %{SOURCE1} %{buildroot}/usr/lib/firmware/qcom/gen71100_gmu.bin
install -Dm644 %{SOURCE2} %{buildroot}/usr/lib/firmware/qcom/gen71100_zap.mbn

%files
/usr/lib/firmware/qcom/gen71100_sqe.fw
/usr/lib/firmware/qcom/gen71100_gmu.bin
/usr/lib/firmware/qcom/gen71100_zap.mbn

%changelog
* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 3.0.304.0-1.local
- Retain the qualified firmware hashes on the real root across reboot.
- Keep all binary and source packages private pending redistribution review.
