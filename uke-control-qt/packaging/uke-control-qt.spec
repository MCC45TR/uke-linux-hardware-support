%global source_name uke-control-qt
Name:           uke-control-gui-qt
Version:        1.1.0
Release:        1%{?dist}
Summary:        Native Kirigami interface for Uke Control
License:        MIT
Source0:        %{source_name}-%{version}.tar.gz
Provides:       uke-control-qt = %{version}-%{release}
Obsoletes:      uke-control-qt < 1.0.0-3
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  extra-cmake-modules >= 6.6
BuildRequires:  cmake(KF6I18n) >= 6.8
BuildRequires:  kf6-kirigami
BuildRequires:  kf6-kirigami-addons >= 1.1
BuildRequires:  cmake(KF6KirigamiAddons) >= 1.1
BuildRequires:  kf6-qqc2-desktop-style
BuildRequires:  cmake(KF6WidgetsAddons) >= 6.6
BuildRequires:  cmake(KF6IconThemes) >= 6.6
BuildRequires:  cmake(KF6ConfigWidgets) >= 6.6
BuildRequires:  desktop-file-utils
BuildRequires:  plasma-breeze-qt6 plasma-integration breeze-icon-theme
BuildRequires:  adwaita-icon-theme
BuildRequires:  pkgconfig(uke-control-client) >= 2.8.0
BuildRequires:  %{_includedir}/uke-control/frontend.hpp
BuildRequires:  uke-control(frontend-sdk-v1) >= 1
Requires:       uke-control >= 2.8.1-11
Requires:       uke-control(frontend-v1) >= 1
Requires:       %{_libexecdir}/uke-control/frontend-request
Requires:       plasma-breeze-qt6
Requires:       plasma-integration
Requires:       breeze-icon-theme
Requires:       qt6-qtdeclarative
Requires:       kf6-kirigami
Requires:       kf6-kirigami-addons >= 1.1
Requires:       kf6-qqc2-desktop-style
Requires:       konsole

%package tests
Summary:        Installed regression checks for the Uke Control Qt interface
Requires:       %{name} = %{version}-%{release}
Requires:       adwaita-icon-theme

%description tests
Optional native regression checks for the packaged interface, icon themes,
typed action routing and isolated simulation. No device changes are performed.

%description
A native Qt 6 Kirigami frontend with a typed C++ adapter for Uke Control.
Daily task pages provide typed requests, asynchronous observations and explicit
review before handing guarded operations to the distribution terminal.

%prep
%autosetup -n %{source_name}-%{version}

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.ControlQt.desktop

%check
%ctest

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-qt
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg

%files tests
%{_libexecdir}/uke-control-qt/tests/
%{_datadir}/uke-control-qt/tests/

%changelog
* Wed Nov 04 2026 Uke contributors - 1.1.0-1
- Redesign Qt navigation around nine task areas and a responsive overview.
- Add a filtered action catalog and staged read, preference and review guides.
- Track read options and stale observations while retaining native Kirigami controls.

* Wed Nov 04 2026 Uke contributors - 1.0.0-8
- Resolve native navigation handle icons across Breeze and Adwaita naming.
- Exercise drawer opening, closing and the themed handle bindings.

* Wed Nov 04 2026 Uke contributors - 1.0.0-7
- Isolate appearance tests from session-wide KDE default configuration paths.

* Wed Nov 04 2026 Uke contributors - 1.0.0-6
- Give native form actions and plain-text information explicit accessible names.
- Clarify terminal read options and clear stale errors before related reads.

* Wed Nov 04 2026 Uke contributors - 1.0.0-5
- Refine daily task guidance and charging/flashlight theme aliases.
- Wait for native page transitions and add optional installed provider reads.

* Wed Nov 04 2026 Uke contributors - 1.0.0-4
- Add semantic theme icons, clearer request controls and daily channel selection.
- Verify alternative themes and ship optional installed UI regression checks.

* Tue Nov 03 2026 Uke contributors - 1.0.0-3
- Publish the Qt GUI under its canonical name with matched backend requirements.

* Mon Nov 02 2026 Uke contributors - 1.0.0-2
- Package the Kirigami interface and run standalone native UI checks.

* Sun Nov 01 2026 Uke contributors - 1.0.0-1
- Add the native Qt Widgets frontend with Breeze style and reviewed requests.
