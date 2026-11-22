Name: uke-gpu-defaults
Version: 0.1.0
Release: 1%{?dist}
Summary: Root-stage Adreno rendering defaults for Uke
License: MIT
BuildArch: noarch
Source0: uke-gpu-modules.conf
Source1: uke-gpu-options.conf
Source2: LICENSE
Requires: kernel-uname-r(7.2.9-senemos-uke)
Requires: mesa-dri-drivers
Requires: /usr/lib/firmware/qcom/gen71100_sqe.fw
Requires: /usr/lib/firmware/qcom/gen71100_gmu.bin
Requires: /usr/lib/firmware/qcom/gen71100_zap.mbn

%description
Load the standard MSM module on the real root using its render-only mode.
The tested boot recipe excludes MSM from the initramfs. This package changes
no clocks, voltages, panel timings, Android partitions or boot selection.

%prep

%build

%install
install -Dm644 %{SOURCE0} %{buildroot}/usr/lib/modules-load.d/uke-gpu.conf
install -Dm644 %{SOURCE1} %{buildroot}/usr/lib/modprobe.d/uke-gpu.conf
install -Dm644 %{SOURCE2} %{buildroot}/usr/share/licenses/%{name}/LICENSE

%files
/usr/lib/modules-load.d/uke-gpu.conf
/usr/lib/modprobe.d/uke-gpu.conf
%license /usr/share/licenses/%{name}/LICENSE

%changelog
* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Load render-only MSM from the real root after firmware installation.
