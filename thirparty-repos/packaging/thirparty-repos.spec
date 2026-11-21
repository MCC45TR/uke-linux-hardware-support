Name:           thirparty-repos
Version:        1.1.0
Release:        3%{?dist}
Summary:        Optional third-party DNF repositories for Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/thirparty-repos
Source0:        LICENSE
Source1:        README.md
Source2:        uke-google-chrome.repo
Source3:        uke-solidcraft.repo
Source4:        chrome-defaults
Source5:        uke-vscode.repo
Source6:        uke-brave.repo
Source7:        uke-mullvad.repo
Source8:        uke-1password.repo
Source9:        uke-orcaslicer.repo
ExclusiveArch:  aarch64
Requires:       %{name}-google-chrome = %{version}-%{release}
Requires:       %{name}-solidcraft = %{version}-%{release}
Requires:       %{name}-vscode = %{version}-%{release}
Requires:       %{name}-brave = %{version}-%{release}
Requires:       %{name}-mullvad = %{version}-%{release}
Requires:       %{name}-1password = %{version}-%{release}
Requires:       %{name}-fedora = %{version}-%{release}
Requires:       %{name}-orcaslicer = %{version}-%{release}

%description
Select optional third-party RPM repositories on Fedora Rawhide AArch64.
Install applications through DNF after repository setup.

%package google-chrome
Summary:        Google Chrome ARM64 repository for DNF
Requires:       fedora-workstation-repositories

%description google-chrome
Register Google's ARM64 repository for Chrome Stable, Beta,
Dev and Canary with Google RPM signature verification.

%package solidcraft
Summary:        SolidCraft ARM64 repository for DNF

%description solidcraft
Register the signed SolidCraft RPM directory in Uke Unstable.

%package vscode
Summary:        Visual Studio Code and Insiders ARM64 repository for DNF

%description vscode
Register Microsoft's signed RPM repository for Visual Studio Code and Insiders.

%package brave
Summary:        Brave Stable ARM64 repository for DNF

%description brave
Register Brave's signed RPM repository for Brave Browser Stable.

%package mullvad
Summary:        Mullvad VPN Stable ARM64 repository for DNF

%description mullvad
Register Mullvad's signed Stable RPM repository.

%package 1password
Summary:        1Password ARM64 repository for DNF

%description 1password
Register the signed 1Password Stable repository, including the ARM64 CLI.

%package fedora
Summary:        Fedora Workstation third-party repository definitions
Requires:       fedora-workstation-repositories

%description fedora
Install Fedora Workstation's curated third-party repository definitions.
Fedora manages the opt-in state for its repository collection.

%package orcaslicer
Summary:        OrcaSlicer ARM64 repository for DNF

%description orcaslicer
Register the signed OrcaSlicer RPM directory in Uke Unstable.

%prep
cp %{SOURCE0} LICENSE
cp %{SOURCE1} README.md

%build

%install
install -Dm0644 %{SOURCE2} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-google-chrome.repo
install -Dm0644 %{SOURCE3} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-solidcraft.repo
install -Dm0644 %{SOURCE5} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-vscode.repo
install -Dm0644 %{SOURCE6} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-brave.repo
install -Dm0644 %{SOURCE7} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-mullvad.repo
install -Dm0644 %{SOURCE8} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-1password.repo
install -Dm0644 %{SOURCE9} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-orcaslicer.repo
for channel in google-chrome-beta google-chrome-unstable google-chrome-canary; do
    install -Dm0644 %{SOURCE4} "%{buildroot}%{_sysconfdir}/default/$channel"
done

%files
%license LICENSE
%doc README.md

%files google-chrome
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-google-chrome.repo
%config(noreplace) %{_sysconfdir}/default/google-chrome-beta
%config(noreplace) %{_sysconfdir}/default/google-chrome-unstable
%config(noreplace) %{_sysconfdir}/default/google-chrome-canary

%files solidcraft
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-solidcraft.repo

%files vscode
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-vscode.repo

%files brave
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-brave.repo

%files mullvad
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-mullvad.repo

%files 1password
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-1password.repo

%files fedora
%license LICENSE

%files orcaslicer
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-orcaslicer.repo

%changelog
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-3
- Include the Brave RPM signing keyring dependency.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-2
- Include the 1Password CLI in the ARM64 repository scope.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-1
- Add VS Code, Brave, Mullvad, 1Password, OrcaSlicer and Fedora third-party repositories.
- Use Fedora Workstation’s Chrome repository default for the Stable package.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Add the signed Google Chrome and SolidCraft repository definitions for AArch64.
