Name:           thirparty-repos
Version:        1.3.0
Release:        1%{?dist}
Summary:        Optional third-party DNF repositories for Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/components/third-party-repos
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
Source10:       uke-onlyoffice.repo
Source11:       uke-rustdesk.repo
Source12:       brave-browser-nightly.repo
Source13:       vivaldi.repo
Source14:       cursor.repo
Source15:       warpdotdev.repo
Source16:       AnyDesk-RPM.repo
Source17:       uke-bruno.repo
Source18:       uke-trilium.repo
Source19:       uke-ferdium.repo
Source20:       uke-chatgpt-preview.repo
Source21:       RPM-GPG-KEY-uke-chatgpt.asc
ExclusiveArch:  aarch64
Requires:       %{name}-google-chrome = %{version}-%{release}
Requires:       %{name}-solidcraft = %{version}-%{release}
Requires:       %{name}-vscode = %{version}-%{release}
Requires:       %{name}-brave = %{version}-%{release}
Requires:       %{name}-mullvad = %{version}-%{release}
Requires:       %{name}-1password = %{version}-%{release}
Requires:       %{name}-fedora = %{version}-%{release}
Requires:       %{name}-orcaslicer = %{version}-%{release}
Requires:       %{name}-onlyoffice = %{version}-%{release}
Requires:       %{name}-rustdesk = %{version}-%{release}
Requires:       %{name}-brave-nightly = %{version}-%{release}
Requires:       %{name}-vivaldi = %{version}-%{release}
Requires:       %{name}-cursor = %{version}-%{release}
Requires:       %{name}-warp = %{version}-%{release}
Requires:       %{name}-anydesk = %{version}-%{release}
Requires:       %{name}-bruno = %{version}-%{release}
Requires:       %{name}-trilium = %{version}-%{release}
Requires:       %{name}-ferdium = %{version}-%{release}
Requires:       %{name}-chatgpt = %{version}-%{release}

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

%package onlyoffice
Summary:        ONLYOFFICE Desktop Editors ARM64 repository for DNF

%description onlyoffice
Register the publisher's signed ONLYOFFICE Desktop Editors RPM repository.

%package rustdesk
Summary:        RustDesk ARM64 repository for DNF

%description rustdesk
Register the signed RustDesk RPM directory in Uke Unstable.

%package brave-nightly
Summary:        Brave Browser Nightly ARM64 repository for DNF

%description brave-nightly
Register the signed Brave Browser Nightly RPM repository.

%package vivaldi
Summary:        Vivaldi Stable ARM64 repository for DNF

%description vivaldi
Register the signed Vivaldi Stable RPM repository.

%package cursor
Summary:        Cursor ARM64 repository for DNF

%description cursor
Register the signed Cursor RPM repository.

%package warp
Summary:        Warp Terminal ARM64 repository for DNF

%description warp
Register the signed Warp Terminal RPM repository.

%package anydesk
Summary:        AnyDesk ARM64 repository for DNF

%description anydesk
Register the signed AnyDesk RPM repository.

%package bruno
Summary:        Bruno ARM64 repository for DNF

%description bruno
Register the signed Bruno RPM repository.

%package trilium
Summary:        Trilium Notes ARM64 repository for DNF

%description trilium
Register the signed Trilium Notes RPM repository.

%package ferdium
Summary:        Ferdium ARM64 repository for DNF

%description ferdium
Register the signed Ferdium RPM repository.

%package chatgpt
Summary:        ChatGPT Desktop Preview ARM64 repository for DNF

%description chatgpt
Register the optional OpenAI Linux preview RPM repository.

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
install -Dm0644 %{SOURCE10} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-onlyoffice.repo
install -Dm0644 %{SOURCE11} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-rustdesk.repo
install -Dm0644 %{SOURCE12} %{buildroot}%{_sysconfdir}/yum.repos.d/brave-browser-nightly.repo
install -Dm0644 %{SOURCE13} %{buildroot}%{_sysconfdir}/yum.repos.d/vivaldi.repo
install -Dm0644 %{SOURCE14} %{buildroot}%{_sysconfdir}/yum.repos.d/cursor.repo
install -Dm0644 %{SOURCE15} %{buildroot}%{_sysconfdir}/yum.repos.d/warpdotdev.repo
install -Dm0644 %{SOURCE16} %{buildroot}%{_sysconfdir}/yum.repos.d/AnyDesk-RPM.repo
install -Dm0644 %{SOURCE17} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-bruno.repo
install -Dm0644 %{SOURCE18} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-trilium.repo
install -Dm0644 %{SOURCE19} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-ferdium.repo
install -Dm0644 %{SOURCE20} %{buildroot}%{_sysconfdir}/yum.repos.d/uke-chatgpt-preview.repo
install -Dm0644 %{SOURCE21} %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-uke-chatgpt.asc
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

%files onlyoffice
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-onlyoffice.repo

%files rustdesk
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-rustdesk.repo

%files brave-nightly
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/brave-browser-nightly.repo

%files vivaldi
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/vivaldi.repo

%files cursor
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/cursor.repo

%files warp
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/warpdotdev.repo

%files anydesk
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/AnyDesk-RPM.repo

%files bruno
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-bruno.repo

%files trilium
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-trilium.repo

%files ferdium
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-ferdium.repo

%files chatgpt
%license LICENSE
%config(noreplace) %{_sysconfdir}/yum.repos.d/uke-chatgpt-preview.repo
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-uke-chatgpt.asc

%changelog
* Sun Nov 22 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.3.0-1
- Add the ARM64 application repositories and the optional ChatGPT preview.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.2.0-1
- Add ONLYOFFICE Desktop Editors and RustDesk ARM64 repositories.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-3
- Include the Brave RPM signing keyring dependency.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-2
- Include the 1Password CLI in the ARM64 repository scope.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-1
- Add VS Code, Brave, Mullvad, 1Password, OrcaSlicer and Fedora third-party repositories.
- Use Fedora Workstation’s Chrome repository default for the Stable package.

* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Add the signed Google Chrome and SolidCraft repository definitions for AArch64.
