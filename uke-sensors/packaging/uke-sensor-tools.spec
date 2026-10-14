Name: uke-sensor-tools
Version: 0.1.0
Release: 1%{?dist}
Summary: Bounded native SSC discovery and sample tools for Uke
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
BuildRequires: gcc-c++
ExclusiveArch: aarch64

%description
Inspect SSC metadata and request bounded standard sensor samples. Requests close
with the QRTR socket. These tools do not run calibration, write persist, start a
service or change boot policy. Front-light and rear-light channel qualification
remains separate from successful protocol parsing.

%prep
%setup -q

%build
%set_build_flags
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror src/ssc-discovery.cpp -o uke-sensor-discovery
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror src/ssc-sample.cpp -o uke-sensor-sample

%check
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-discovery.cpp -o discovery-test
./discovery-test
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-sample.cpp -o sample-test
./sample-test

%install
install -Dm755 uke-sensor-discovery %{buildroot}%{_bindir}/uke-sensor-discovery
install -Dm755 uke-sensor-sample %{buildroot}%{_bindir}/uke-sensor-sample

%files
%{_bindir}/uke-sensor-discovery
%{_bindir}/uke-sensor-sample

%changelog
* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Package existing bounded native discovery and sample tools for controlled trials.
- Run parser, source, sample-dimension and finite-value fixtures at build time.
