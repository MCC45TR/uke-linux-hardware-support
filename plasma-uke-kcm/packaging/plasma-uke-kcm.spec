Name: plasma-uke-kcm
Version: 0.2.0
Release: 1%{?dist}
Summary: Read-only Uke tablet overview in the original KDE System Settings
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64 x86_64
BuildRequires: gcc-c++ cmake cmake-rpm-macros make extra-cmake-modules gettext
BuildRequires: cmake(Qt6Core) >= 6.8 cmake(Qt6Qml) cmake(Qt6Quick) cmake(Qt6Test)
BuildRequires: cmake(KF6KCMUtils) cmake(KF6CoreAddons) cmake(KF6I18n)
Requires: uke-control >= 2.4.0
Requires: kf6-kcmutils kf6-kirigami
Recommends: systemsettings

%description
Native KDE configuration module for independent, bounded read-only component
observations. It uses the existing Uke command API. It does not install a root
daemon, perform maintenance writes or infer physical support from enumeration.

%prep
%autosetup

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%check
%ctest

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_uke.so
%{_datadir}/applications/kcm_uke.desktop
%{_datadir}/locale/tr/LC_MESSAGES/kcm_uke.mo

%changelog
* Fri Oct 30 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.2.0-1
- Add an original-KDE read-only overview with independent bounded queries.
