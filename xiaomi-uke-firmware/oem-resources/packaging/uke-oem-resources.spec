Name: uke-oem-resources
Version: 1.0.0
Release: 1%{?dist}
Summary: Optional read-only Uke OEM resource access
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.xz
BuildArch: noarch
BuildRequires: bash coreutils diffutils gawk jq util-linux-core systemd-rpm-macros
Requires: bash coreutils diffutils gawk jq util-linux util-linux-core ripgrep
%{?systemd_requires}

%description
Expose separate OEM firmware banks and verified linear vendor/ODM references
through read-only loop devices. Extract bounded DTBO references without applying
an overlay. Prepare only byte-identical SSC defaults for an optional native
consumer; retain packaged firmware and factory persist unchanged.
No OEM bytes, private calibration or Python payload is included.

%prep
%autosetup

%build

%check
bash tests/metadata.sh

%install
install -Dm755 src/resources.sh %{buildroot}%{_libexecdir}/uke-oem-resources/resources.sh
install -Dm755 src/read-super.sh %{buildroot}%{_libexecdir}/uke-oem-resources/read-super.sh
install -Dm644 packaging/uke-oem-resources.service %{buildroot}%{_unitdir}/uke-oem-resources.service
install -Dm644 packaging/oem-sources.conf %{buildroot}%{_unitdir}/uke-ssc-cache.service.d/oem-sources.conf

%post
%systemd_post uke-oem-resources.service

%preun
%systemd_preun uke-oem-resources.service

%postun
%systemd_postun uke-oem-resources.service

%files
%license LICENSE
%doc README.md
%{_libexecdir}/uke-oem-resources
%{_unitdir}/uke-oem-resources.service
%{_unitdir}/uke-ssc-cache.service.d/oem-sources.conf

%changelog
* Sat Oct 31 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Add optional read-only OEM references with checksum and extent validation.
- Retain package fallback and separate A/B banks without applying DT overlays.
