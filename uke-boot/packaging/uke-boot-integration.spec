%global debug_package %{nil}
Name: uke-boot-integration
Version: 0.1.0
Release: 3%{?dist}
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

%package -n uke-esp32-cdc
Summary: Native ESP32-S3 CDC journal forwarding for the Uke Core debug profile
Requires: %{name} = %{version}-%{release}
Requires: systemd senemos-native-runtime(libstdc++)
Conflicts: python(abi) python3-libs pypy pypy3
%description -n uke-esp32-cdc
Native raw-TTY configuration and real-root journal forwarding to an explicitly
configured ttyACM USB-host port. Core composition may apply the separate original
systemd debug-shell override for HID-driven VT2 commands and journal output.
Installation activates no service, gadget, boot entry or partition write.

%prep
%setup -q
%build
%{__cxx} %{optflags} -std=c++17 -ffile-prefix-map=$PWD=. src/boot-status.cpp -o uke-boot-status %{build_ldflags}
%{__cxx} %{optflags} -std=c++17 -ffile-prefix-map=$PWD=. src/esp32-cdc.cpp -o uke-esp32-cdc %{build_ldflags}
%install
install -Dm755 uke-boot-status %{buildroot}%{_bindir}/uke-boot-status
install -d %{buildroot}%{_datadir}/senemos/boot/uke
install -m644 src/profiles/* %{buildroot}%{_datadir}/senemos/boot/uke/
install -Dm644 src/units/uke-cdc-acm.service %{buildroot}%{_unitdir}/uke-cdc-acm.service
install -Dm755 uke-esp32-cdc %{buildroot}%{_libexecdir}/senemos-uke/uke-esp32-cdc
install -Dm644 src/units/uke-esp32-cdc-log.service %{buildroot}%{_unitdir}/uke-esp32-cdc-log.service
%files
%license LICENSE
%doc README.md
%{_bindir}/uke-boot-status
%{_datadir}/senemos/boot/uke
%{_unitdir}/uke-cdc-acm.service

%files -n uke-esp32-cdc
%license LICENSE
%{_libexecdir}/senemos-uke/uke-esp32-cdc
%{_unitdir}/uke-esp32-cdc-log.service

%changelog
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Add native ESP32-S3 USB-host journal forwarding and HID/VT2 Core inputs.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Add native CDC prerequisite inspection and opt-in Core debug integration.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Provide explicit image inputs and native read-only prerequisite inspection.
