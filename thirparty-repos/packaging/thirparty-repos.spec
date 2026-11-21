Name:           thirparty-repos
Version:        1.0.0
Release:        1%{?dist}
Summary:        Optional third-party DNF repositories for Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/thirparty-repos
Source0:        LICENSE
Source1:        README.md
Source2:        uke-google-chrome.repo
Source3:        uke-solidcraft.repo
Source4:        chrome-defaults
ExclusiveArch:  aarch64
Requires:       %{name}-google-chrome = %{version}-%{release}
Requires:       %{name}-solidcraft = %{version}-%{release}

%description
Select the optional Google Chrome and SolidCraft RPM repositories on Fedora
Rawhide AArch64. Install applications through DNF after repository setup.

%package google-chrome
Summary:        Google Chrome ARM64 repository for DNF

%description google-chrome
Register Google's ARM64 repository for Chrome Stable, Beta,
Dev and Canary with Google RPM signature verification.

%package solidcraft
Summary:        SolidCraft ARM64 repository for DNF

%description solidcraft
Register the signed SolidCraft RPM directory in Uke Unstable.

%prep
cp %{SOURCE0} LICENSE
cp %{SOURCE1} README.md

%build

%install
install -Dm0644 %{SOURCE2} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-google-chrome.repo
install -Dm0644 %{SOURCE3} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-solidcraft.repo
for channel in google-chrome google-chrome-beta google-chrome-unstable google-chrome-canary; do
    install -Dm0644 %{SOURCE4} "%{buildroot}%{_sysconfdir}/default/$channel"
done

%files
%license LICENSE
%doc README.md

%files google-chrome
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-google-chrome.repo
%config(noreplace) %{_sysconfdir}/default/google-chrome
%config(noreplace) %{_sysconfdir}/default/google-chrome-beta
%config(noreplace) %{_sysconfdir}/default/google-chrome-unstable
%config(noreplace) %{_sysconfdir}/default/google-chrome-canary

%files solidcraft
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-solidcraft.repo

%changelog
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Add the signed Google Chrome and SolidCraft repository definitions for AArch64.
