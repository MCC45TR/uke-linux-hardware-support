Name: uke-local-sensor-data
Version: 3.0.304.0
Release: 1.local%{?dist}
Summary: Device-local OEM sensor inputs for Uke development
License: LicenseRef-Device-Local-Redistribution-Unreviewed
BuildArch: noarch
Source0: sensor-data.tar.xz

%description
The retained Uke OEM sensor configuration and DSP plug-in. No personal persist
registry is packaged. Calibration is read separately from the read-only device
partition. These binary/source packages must remain private until redistribution
permission is established. Installed files are inputs, not hardware acceptance.

%prep
echo '82e354ca9d3fc5aa4a966a9cae7e06cc265f277b2f17b3224ed89e9cdc9594ac  %{SOURCE0}' | sha256sum -c -
%setup -q -c -T
tar -xf %{SOURCE0}
echo 'cf8c9ca1b51be7d79573b08936f5e0c506b60673ea14058c0fc6dbe358184e4d  sns_tppe.so' | sha256sum -c -
echo '1955666f46427f102b2bf19a0018123fbc27cf6951d531a5e623823a0ed1ea35  sns_reg.conf' | sha256sum -c -
test "$(find odm -type f | wc -l)" -eq 68
if find . -type l -o -name '*.py' | grep -q .; then exit 1; fi

%build

%install
mkdir -p %{buildroot}/usr/share/uke-sensors/odm
install -m644 odm/* %{buildroot}/usr/share/uke-sensors/odm/
install -Dm644 sns_reg.conf %{buildroot}/usr/share/uke-sensors/sns_reg.conf
install -Dm644 sns_tppe.so %{buildroot}/usr/lib/firmware/qcom/uke/sensors/sns_tppe.so

%files
/usr/share/uke-sensors/
/usr/lib/firmware/qcom/uke/sensors/sns_tppe.so

%changelog
* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 3.0.304.0-1.local
- Retain the verified OEM inputs independently of RAM laboratory directories.
- Exclude personal calibration and keep redistribution unapproved.
