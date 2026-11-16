Name: uke-control-setup-source-check
Version: 0.1
Release: 3%{?dist}
Summary: Native qualification of the temporary Uke first-setup USB protocol
License: MIT
Source0: setup-cdc.tar.xz
ExclusiveArch: aarch64
%global debug_package %{nil}
BuildRequires: gcc-c++ clang pkgconfig(jsoncpp) binutils bash coreutils findutils tar xz

%description
Essential build-only qualification of the actual first-setup policy and USB
protocol source with private synthetic files and a real Linux PTY. Tests keep
service, gadget and reboot operations behind asserted fixed-argument stubs.
The package contains text receipts only. It installs no daemon, runtime ELF,
hardware helper or device hook and is not intended for the tablet.

%prep
%setup -q -n setup-cdc
sha256sum -c --strict --quiet SHA256SUMS

%build
if ! bash tests/setup-cdc.sh "$PWD" > native-results.txt 2>&1; then
 tail -n 80 native-results.txt
 exit 1
fi
cat native-results.txt
sha256sum src/{setup-cdc.cpp,setup-cdc.hpp,backend.hpp,sysfs.hpp} tests/{setup-cdc.cpp,setup-cdc.sh} > qualified-source.sha256
rpm -q gcc-c++ clang glibc jsoncpp-devel > distribution-tools.txt

%check
test "$(grep -c '^PASS: actual setup policy' native-results.txt)" = 4
sha256sum -c --strict --quiet qualified-source.sha256

%install
install -d %{buildroot}%{_datadir}/%{name}
install -m644 native-results.txt qualified-source.sha256 distribution-tools.txt %{buildroot}%{_datadir}/%{name}/
install -Dm644 LICENSE %{buildroot}%{_licensedir}/%{name}/LICENSE

%files
%{_datadir}/%{name}
%license %{_licensedir}/%{name}/LICENSE

%changelog
* Mon Nov 16 2026 Uke contributors - 0.1-3
- Express the bounded claim write with explicit array indexing for strict Clang builds.

* Mon Nov 16 2026 Uke contributors - 0.1-2
- Retain the shared-library boundary in the GCC fixture without changing production code or warning enforcement.

* Mon Nov 16 2026 Uke contributors - 0.1-1
- Qualify exact commands, native PTY transport, ownership refusal, completion and preserved CDC intent without hardware operations.
