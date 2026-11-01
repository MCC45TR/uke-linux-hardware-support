Name:           uke-control-qt
Version:        1.0.0
Release:        1%{?dist}
Summary:        Native Qt Widgets interface for Uke Control
License:        MIT
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  pkgconfig(uke-control-client)
Requires:       uke-control >= 2.8.0
Requires:       plasma-breeze-qt6
Requires:       konsole

%description
A native C++ Qt 6 Widgets frontend for the existing Uke Control client.
Daily task pages provide typed requests, asynchronous observations and explicit
review before handing guarded operations to the distribution terminal.

%prep
%autosetup

%build
%cmake -DBUILD_TESTING=OFF
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-qt
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg

%changelog
* Sun Nov 01 2026 Uke contributors - 1.0.0-1
- Add the native Qt Widgets frontend with Breeze style and reviewed requests.
