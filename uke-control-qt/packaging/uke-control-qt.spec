%global source_name uke-control-qt
Name:           uke-control-gui-qt
Version:        1.6.0
Release:        6%{?dist}
Summary:        Native Kirigami interface for Uke Control
License:        MIT
Source0:        %{source_name}-%{version}.tar.gz
Provides:       uke-control-qt = %{version}-%{release}
Obsoletes:      uke-control-qt < 1.0.0-3
BuildRequires:  gcc-c++
BuildRequires:  zlib-devel
BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  gettext
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtconnectivity-devel
BuildRequires:  extra-cmake-modules >= 6.6
BuildRequires:  cmake(KF6I18n) >= 6.8
BuildRequires:  kf6-kirigami
BuildRequires:  kf6-kirigami-addons >= 1.9
BuildRequires:  cmake(KF6KirigamiAddons) >= 1.9
BuildRequires:  kf6-qqc2-desktop-style
BuildRequires:  cmake(KF6WidgetsAddons) >= 6.6
BuildRequires:  cmake(KF6IconThemes) >= 6.6
BuildRequires:  cmake(KF6ConfigWidgets) >= 6.6
BuildRequires:  desktop-file-utils
BuildRequires:  plasma-breeze-qt6 plasma-integration breeze-icon-theme
BuildRequires:  adwaita-icon-theme
BuildRequires:  pkgconfig(uke-control-client) >= 2.8.0
BuildRequires:  uke-control-client-devel >= 2.8.1-23
BuildRequires:  %{_includedir}/uke-control/frontend.hpp
BuildRequires:  uke-control(frontend-sdk-v1) >= 1
Requires:       uke-control >= 2.8.1-23
Requires:       uke-control(update-status-v1) >= 1
Requires:       uke-control(slot-selection-v1) >= 1
Requires:       uke-control(frontend-v1) >= 1
Requires:       %{_libexecdir}/uke-control/frontend-request
Requires:       plasma-breeze-qt6
Requires:       plasma-integration
Requires:       breeze-icon-theme
Requires:       qt6-qtdeclarative
Requires:       qt6-qtconnectivity
Requires:       neard >= 0.14
Requires:       polkit
Requires:       uke-charge-control >= 1.4.1-7
Requires:       kf6-kirigami
Requires:       kf6-kirigami-addons >= 1.9
Requires:       kf6-qqc2-desktop-style
Requires:       konsole
Requires:       uke-waydroid-installer = %{version}-%{release}

%package -n uke-waydroid-installer
Summary:        Native graphical Waydroid installation backend
Requires:       polkit systemd dnf5
Requires:       coreutils rpm
Requires:       (uke-waydroid-integration >= 0.1.0-8 if waydroid)

%description -n uke-waydroid-installer
Install original Fedora Waydroid packages and official GAPPS or Vanilla images
after desktop authorization. A system service retains installation progress
when the graphical window closes. Existing Android data and editions are kept.
The original Waydroid package may retain its distribution-provided Python
helpers; this package contains only native C++ code and no project Python.

%post -n uke-waydroid-installer
%{_bindir}/systemctl daemon-reload || :

%postun -n uke-waydroid-installer
%{_bindir}/systemctl daemon-reload || :

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
%{_bindir}/uke-kernel-status
%{_libexecdir}/uke-control-qt/kernel-information
%{_libexecdir}/uke-control-qt/frequency-control
%{_libexecdir}/uke-control-qt/resource-control
%{_libexecdir}/uke-control-qt/application-control
%{_datadir}/polkit-1/actions/org.senemos.uke.resource-control.policy
%{_datadir}/polkit-1/actions/org.senemos.uke.frequency-control.policy
%{_datadir}/polkit-1/actions/org.senemos.uke.qt-kernel-information.policy
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg
%{_datadir}/locale/*/LC_MESSAGES/uke-control-qt.mo

%files tests
%{_libexecdir}/uke-control-qt/tests/
%{_datadir}/uke-control-qt/tests/

%files -n uke-waydroid-installer
%license LICENSE
%{_libexecdir}/uke-control-qt/waydroid-installer
%{_datadir}/polkit-1/actions/org.senemos.uke.waydroid-installer.policy
%{_prefix}/lib/systemd/system/uke-waydroid-install@.service

%changelog
* Thu Nov 05 2026 Uke contributors - 1.6.0-6
- Bound transient telemetry identities and separate restarted application counters.

* Thu Nov 05 2026 Uke contributors - 1.6.0-5
- Keep current readings in chart cards and disclose graph selection on demand.

* Thu Nov 05 2026 Uke contributors - 1.6.0-4
- Initialize hidden numeric editors for categorical and unavailable providers.

* Thu Nov 05 2026 Uke contributors - 1.6.0-3
- Bound application enumeration and remember the last manually selected preset.
- Keep application CPU/memory graphs beside resource controls and remove thermal mirrors by provider.

* Thu Nov 05 2026 Uke contributors - 1.6.0-2
- Inspect bounded DRM client counters and deduplicate hwmon temperature sources.
- Export bounded session reports and exercise new resource controls in QML fixtures.

* Thu Nov 05 2026 Uke contributors - 1.6.0-1
- Add kernel resource pages, pressure and I/O graphs, residency and capability explanations.
- Add reviewed runtime policies and user application resource presets with readback.
- Provide readable kernel resource information in the terminal.

* Thu Nov 05 2026 Uke contributors - 1.5.0-5
- Require the qualified ARM64-only media integration whenever Waydroid is installed.

* Thu Nov 05 2026 Uke contributors - 1.5.0-4
- Retain the queued telemetry-result race fix and its isolated regression check.

* Thu Nov 05 2026 Uke contributors - 1.5.0-3
- Retain the latest delivered Kernel Manager review, graph and initial-read fixes.

* Thu Nov 05 2026 Uke contributors - 1.5.0-2
- Verify active startup and reload packaged settings when repairing an installation.
- Refuse implicit kernel-profile replacement and preserve unknown Android editions.

* Thu Nov 05 2026 Uke contributors - 1.5.0-1
- Add graphical GAPPS/Vanilla installation, durable progress and native launch.
- Preserve existing Android data and expose private Google registration help.

* Thu Nov 05 2026 Uke contributors - 1.4.0-4
- Keep the authorized initial device read when monitoring pauses and wait for delivered results in live checks.
- Keep graphs near the current measurements and disclose monitoring/source details on demand.

* Thu Nov 05 2026 Uke contributors - 1.4.0-3
- Limit review dialogs and sensor selection to the visible settings page.
- Exercise a reviewed GPU request after visiting the CPU page.

* Thu Nov 05 2026 Uke contributors - 1.4.0-2
- Offer explicit advertised limits and preserve fractional MHz in frequency controls.
- Show the selected sensor value and source state beside its graph.

* Thu Nov 05 2026 Uke contributors - 1.4.0-1
- Add Senemos Kernel Manager with live CPU, GPU and sensor graphs beside their controls.
- Restrict reviewed frequency requests to advertised policies with stale-state checks and readback.
- Combine memory/swap and sleep/wake observations with their related preferences.

* Wed Nov 04 2026 Uke contributors - 1.3.0-6
- Convert the observed Qt role value to the shared SDK's string type.

* Wed Nov 04 2026 Uke contributors - 1.3.0-5
- Keep role and reader controls above detailed cards and start role requests from observed state.
- Condense accessory details behind native disclosure controls.

* Wed Nov 04 2026 Uke contributors - 1.3.0-4
- Enforce read-only mode for explicit reader-service startup in C++ and QML.

* Wed Nov 04 2026 Uke contributors - 1.3.0-3
- Align the executable version and add opt-in rendering of actual installed kernel information.

* Wed Nov 04 2026 Uke contributors - 1.3.0-2
- Require the original neard backend and offer explicit authorized service startup.
- Bound privileged reads independently and keep cancellation asynchronous.

* Wed Nov 04 2026 Uke contributors - 1.3.0-1
- Show kernel-backed dock details and verified charging settings through a fixed read-only helper.
- Add bounded explicit NDEF reading through the original Qt NFC backend and distinguish the built-in tag.
- Expose the charge-limit request and explain automatic Type-C handling without disabling the connection.

* Wed Nov 04 2026 Uke contributors - 1.2.0-5
- Bind chart colours to the actual inherited Kirigami theme context.

* Wed Nov 04 2026 Uke contributors - 1.2.0-4
- Improve plot-axis contrast using native foreground colours.
- Show single preferences directly and preserve drafts when refreshing observations.

* Wed Nov 04 2026 Uke contributors - 1.2.0-3
- Keep incomplete boot evidence visible and block daily slot changes until consistent.
- Preserve topic-read ownership during rapid navigation and retain explicit other-slot requests.

* Wed Nov 04 2026 Uke contributors - 1.2.0-2
- Preserve native Back navigation and avoid invalid initial file selections.
- Refine reported measurements, unsupported-state handling and topic information density.

* Wed Nov 04 2026 Uke contributors - 1.2.0-1
- Replace command-oriented daily pages with grouped settings topics and information tiles.
- Add bounded read-only UPower battery history and semantic preference controls.

* Wed Nov 04 2026 Uke contributors - 1.1.0-2
- Add readable A/B kernel status cards and reviewed next-boot selection guides.
- Require the matched status/selection backend and retain native field wrapping.

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
