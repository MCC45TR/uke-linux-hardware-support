%global debug_package %{nil}
Name: uke-boot-integration
Version: 0.1.0
Release: 2%{?dist}
Summary: Uke image configuration and read-only boot prerequisite inspection
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-boot
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros
Requires: senemos-native-runtime(libstdc++)
Requires: kmod bash systemd
Conflicts: python(abi) python3-libs pypy pypy3

%description
Reviewed Uke partition-role, command-line and dracut input templates, with a
native read-only prerequisite inspection command. Templates require explicit
host-composer application; installation does not regenerate initramfs, select
a default boot entry, alter EFI variables or write Android partitions.
No UEFI firmware or proprietary firmware blob is included. Package success
does not establish RAM/UFS/firmware handoff or physical tablet boot.

%prep
%setup -q
%build
%{__cxx} %{optflags} -std=c++17 -ffile-prefix-map=$PWD=. src/boot-status.cpp -o uke-boot-status %{build_ldflags}
%install
install -Dm755 uke-boot-status %{buildroot}%{_bindir}/uke-boot-status
install -d %{buildroot}%{_datadir}/senemos/boot/uke
install -m644 src/profiles/* %{buildroot}%{_datadir}/senemos/boot/uke/
install -Dm644 src/units/uke-cdc-acm.service %{buildroot}%{_unitdir}/uke-cdc-acm.service
%files
%license LICENSE
%doc README.md
%{_bindir}/uke-boot-status
%{_datadir}/senemos/boot/uke
%{_unitdir}/uke-cdc-acm.service

%changelog
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Add native CDC prerequisite inspection and opt-in Core debug integration.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Provide explicit image inputs and native read-only prerequisite inspection.
