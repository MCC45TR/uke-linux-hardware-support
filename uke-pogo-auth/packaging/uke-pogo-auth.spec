Name: uke-pogo-auth
Version: 0.1
Release: 1%{?dist}
Summary: Bounded native Uke pogo keyboard authentication candidate
License: MIT AND BSD-2-Clause AND BSD-3-Clause
Source0: %{name}-%{version}.tar.gz
Source1: minkipc.tar.gz
Source2: quic-teec.tar.gz
Source3: qcbor.tar.gz
ExclusiveArch: aarch64
BuildRequires: gcc gcc-c++ openssl-devel systemd-rpm-macros
BuildRequires: pkgconfig(libsystemd)
Requires: uke-cdsp-pogo-firmware >= 0.2
Provides: bundled(minkipc) = 1.0.0
Provides: bundled(quic-teec) = 0.1.0
Provides: bundled(qcbor) = 1.7.0

%description
Original C++ client for the bounded Uke kernel authentication handoff. Uses
fixed original MINK/QCOMTEE/QCBOR library sources and only the exact locally
provisioned Uke TA. Includes no OEM firmware and no system RPMB listener.
The static systemd unit requires explicit manual startup. Physical token
acceptance and exact Uke TA/challenge compatibility remain under qualification.

%prep
%setup -q
printf '2da103029796465773fa77746cf10799ff1fb43c62ba8dbb1efc51b62e3dd36f  %{SOURCE1}\n887ace80e98c0dc67163a44fdd280c9a9cf0e46a9671ebf33643def261fc405a  %{SOURCE2}\n93fa4b2e56a2fe5af32be5e3c05a95d4563daea4b127023d6f77beb2274bf6ae  %{SOURCE3}\n' | sha256sum -c --strict
mkdir -p upstream/minkipc upstream/quic-teec upstream/qcbor
tar -xzf %{SOURCE1} --strip-components=1 -C upstream/minkipc
tar -xzf %{SOURCE2} --strip-components=1 -C upstream/quic-teec
tar -xzf %{SOURCE3} --strip-components=1 -C upstream/qcbor

%build
%set_build_flags
bash uke-pogo-auth/packaging/build.sh

%check
bash uke-pogo-auth/tests/check.sh upstream/minkipc

%install
install -Dm755 build/uke-pogo-auth %{buildroot}%{_libexecdir}/uke-pogo-auth
install -Dm644 uke-pogo-auth/packaging/uke-pogo-auth.service %{buildroot}%{_unitdir}/uke-pogo-auth.service
install -Dm644 uke-pogo-auth/README.md %{buildroot}%{_docdir}/%{name}/README.md
install -Dm644 uke-pogo-auth/LICENSE %{buildroot}%{_licensedir}/%{name}/MIT-LICENSE.txt
install -Dm644 upstream/minkipc/LICENSE.txt %{buildroot}%{_licensedir}/%{name}/minkipc-LICENSE.txt
install -Dm644 upstream/quic-teec/LICENSE.txt %{buildroot}%{_licensedir}/%{name}/quic-teec-LICENSE.txt
install -Dm644 upstream/qcbor/LICENSE %{buildroot}%{_licensedir}/%{name}/qcbor-LICENSE.txt
install -Dm644 upstream/quic-teec/libqcomtee/src/linux/tee.h %{buildroot}%{_licensedir}/%{name}/LINARO-tee-interface-notice.h

%files
%{_libexecdir}/uke-pogo-auth
%{_unitdir}/uke-pogo-auth.service
%doc %{_docdir}/%{name}/README.md
%license %{_licensedir}/%{name}/*

%changelog
* Wed Nov 04 2026 Uke Linux contributors <75160848+MCC45TR@users.noreply.github.com> - 0.1-1
- Add original bounded authentication, strict TA error handling and epoch/challenge admission.
- Build original pinned libraries without a compiler download or OEM redistribution.
