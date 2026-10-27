Name:           uke-speaker-firmware
Version:        3.0.304.0
Release:        1%{?dist}
Summary:        Stock Uke FS19xx speaker preset
License:        Apache-2.0
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.xz
BuildArch:      noarch
BuildRequires:  coreutils
AutoReqProv:    no
Provides:       /usr/lib/firmware/qcom/uke/audio/fs19xx.fsm

%description
Unmodified generic FS19xx preset from Uke Global OS3.0.304.0. The OEM ODM
notice maps this file to its Apache-2.0 entry. Device-private calibration,
persist contents and executable helpers are not included. Hardware playback
and protection effectiveness require separate validation.

%prep
%autosetup

%build

%install
install -Dm0644 fs19xx.fsm %{buildroot}%{_prefix}/lib/firmware/qcom/uke/audio/fs19xx.fsm

%check
echo 'f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b  fs19xx.fsm' | sha256sum -c -
cmp fs19xx.fsm %{buildroot}%{_prefix}/lib/firmware/qcom/uke/audio/fs19xx.fsm

%files
%license LICENSE.Apache-2.0 odm-NOTICE.xml.gz
%doc README.md provenance.json
%config(noreplace) %{_prefix}/lib/firmware/qcom/uke/audio/fs19xx.fsm

%changelog
* Tue Oct 27 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 3.0.304.0-1
- Package the unmodified stock preset with its exact OEM notice mapping.
- Preserve an existing locally modified preset during upgrades.
