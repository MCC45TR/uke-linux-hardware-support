Name: uke-local-touch-data
Version: 1
Release: 1%{?dist}
Summary: Private qualified NT36532 inputs for one Uke installation
License: LicenseRef-Local-Private-Data
BuildArch: noarch
Source0: normal.bin
Source1: config.ini
Source2: reference.bin
Requires: uke-touchscreen

%description
Local-only firmware, OEM geometry and a qualified private reference packet.
Never upload this package, its sources or extracted payload to COPR or GitHub.
Installation does not alter OEM persist or Android partitions.

%prep
%build
%install
install -d -m700 %{buildroot}/var/lib/uke-touch
install -m600 %{SOURCE0} %{buildroot}/var/lib/uke-touch/normal.bin
install -m600 %{SOURCE1} %{buildroot}/var/lib/uke-touch/config.ini
install -m600 %{SOURCE2} %{buildroot}/var/lib/uke-touch/reference.bin

%files
%dir %attr(0700,root,root) /var/lib/uke-touch
%attr(0600,root,root) /var/lib/uke-touch/normal.bin
%attr(0600,root,root) /var/lib/uke-touch/config.ini
%attr(0600,root,root) /var/lib/uke-touch/reference.bin
