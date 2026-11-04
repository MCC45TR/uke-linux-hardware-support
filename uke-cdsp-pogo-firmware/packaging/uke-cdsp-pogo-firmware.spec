Name: uke-cdsp-pogo-firmware
Version: 0.2
Release: 5%{?dist}
Summary: Read-only device-local Uke CDSP and pogo firmware provisioning
License: MIT
Source0: %{name}-%{version}.tar.gz
BuildRequires: gcc-c++
BuildRequires: openssl-devel
Requires: e2fsprogs
ExclusiveArch: aarch64

%description
Native C++ helper that reads fixed stock CDSP files and the exact Uke MCU
image from local read-only stock partitions. Publishes only non-running,
versioned firmware families. Contains no OEM firmware or calibration.

%prep
%setup -q

%build
%set_build_flags
g++ %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror -Wno-misleading-indentation uke-cdsp-pogo-firmware/src/import.cpp %{build_ldflags} -lcrypto -o uke-cdsp-pogo-firmware-bin

%check
bash uke-cdsp-pogo-firmware/tests/check.sh

%install
install -Dm755 uke-cdsp-pogo-firmware-bin %{buildroot}%{_libexecdir}/uke-cdsp-pogo-firmware
install -Dm644 uke-cdsp-pogo-firmware/README.md %{buildroot}%{_docdir}/%{name}/README.md

%files
%{_libexecdir}/uke-cdsp-pogo-firmware
%doc %{_docdir}/%{name}/README.md

%changelog
* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.2-5
- Pin complete current Uke stock identities verified by two independent readers.
- Keep historical reference hashes separate and preserve secure-loader qualification.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.2-4
- Hash only the bounded declared stock FAT volume in one-MiB memory chunks.
- Distinguish source-volume identity from unused partition tail without a backup.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.2-3
- Report bounded fixed-file identities for read-only stock mismatch investigation.
- Preserve complete-image admission for every import and preflight.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.2-2
- Resolve fixed firmware names through validated VFAT long-name entries.
- Test alias/checksum/order/padding failures without relaxing image hashes.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.2-1
- Reconstruct only the exact Uke authentication image from bounded read-only splits.
- Keep trusted-app loading separate and scope candidate imports to unstable ABIs.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1-2
- Admit the observed modem and super LUNs only below the exact Uke UFS controller.

* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1-1
- Add bounded read-only stock firmware provisioning without OEM redistribution.
