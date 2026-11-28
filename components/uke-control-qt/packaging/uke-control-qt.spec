%global debug_package %{nil}
%global source_name uke-control-qt
%global sdk_evr 2.17.0-64.fc46
%global sdk_base https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u
Name:           uke-control-gui-qt
Version:        1.6.0
Release:        27%{?dist}
Summary:        Native Kirigami interface for Uke Control
License:        MIT
Source0:        %{source_name}-%{version}.tar.gz
%ifarch aarch64
Source1:        %{sdk_base}/uke-control-%{sdk_evr}.aarch64.rpm
Source2:        %{sdk_base}/uke-control-client-devel-%{sdk_evr}.aarch64.rpm
%endif
Provides:       uke-control-qt = %{version}-%{release}
Obsoletes:      uke-control-qt < 1.0.0-3
BuildRequires:  gcc-c++
BuildRequires:  zlib-devel
BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  gettext
BuildRequires:  layer-shell-qt-devel pkgconfig(libdrm)
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
%ifarch aarch64
# The Rawhide quic-teec build currently requires an unpublished libcbor SONAME.
# Use the exact signed Control SDK payload without installing the Control RPM.
BuildRequires:  cpio
BuildRequires:  jsoncpp
%else
BuildRequires:  pkgconfig(uke-control-client) >= 2.8.0
BuildRequires:  uke-control-client-devel >= 2.8.1-23
BuildRequires:  %{_includedir}/uke-control/frontend.hpp
BuildRequires:  uke-control(frontend-sdk-v1) >= 1
%endif
Requires:       uke-control >= 2.17.0-69
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
Requires:       systemd dnf5 coreutils rpm
Requires:       (uke-waydroid-integration >= 0.1.0-8 if waydroid)
Provides:       uke-waydroid-installer = %{version}-%{release}
Provides:       uke-waydroid-installer%{?_isa} = %{version}-%{release}
Provides:       uke-waydroid-installer = 1.6.0-7%{?dist}
Provides:       uke-waydroid-installer%{?_isa} = 1.6.0-7%{?dist}
Obsoletes:      uke-waydroid-installer < 1.6.0-8%{?dist}
Obsoletes:      uke-control-gui-qt-tests < 1.6.0-7%{?dist}

%description
A native Qt 6 Kirigami frontend with a typed C++ adapter for Uke Control.
Daily task pages provide typed requests, asynchronous observations and explicit
review before handing guarded operations to the distribution terminal.
The interface also includes its native Waydroid installer.

%post
%{_bindir}/systemctl daemon-reload || :

%postun
%{_bindir}/systemctl daemon-reload || :

%prep
%autosetup -n %{source_name}-%{version}
%ifarch aarch64
sha256sum -c <<EOF
8268a5540b41324ce3b2f19a08592522bf51a026f16c9dcb078889df8edcfff7  %{SOURCE1}
150888ae153fda7133f9a37b8dd17361339577e58aeda799f891495bd76c8c2d  %{SOURCE2}
EOF
mkdir -p .control-sdk/{payload,src,build}
(
    set -o pipefail
    cd .control-sdk/payload
    rpm2cpio %{SOURCE1} | cpio -id --quiet './usr/lib64/libuke-control-client.so.1'
    rpm2cpio %{SOURCE2} | cpio -id --quiet './usr/include/uke-control/client.hpp' './usr/include/uke-control/frontend.hpp'
)
mv .control-sdk/payload/usr/include/uke-control/{client,frontend}.hpp .control-sdk/src/
mv .control-sdk/payload/usr/lib64/libuke-control-client.so.1 .control-sdk/build/
%endif

%build
%ifarch aarch64
%cmake -DBUILD_TESTING=ON -DUKE_CONTROL_SOURCE="$PWD/.control-sdk"
%else
%cmake -DBUILD_TESTING=ON
%endif
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.ControlQt.desktop

%check
%ctest
# Retain every build-time test; completed checks are not runtime payload.
for directory in "%{buildroot}%{_libexecdir}/uke-control-qt/tests" "%{buildroot}%{_datadir}/uke-control-qt/tests"; do
    test -d "$directory" && test ! -L "$directory"
    find "$directory" -depth -delete
done

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-qt
%{_bindir}/uke-kernel-status
%{_libexecdir}/uke-control-qt/kernel-information
%{_libexecdir}/uke-control-qt/package-task
%{_libexecdir}/uke-control-qt/frequency-control
%{_libexecdir}/uke-control-qt/resource-control
%{_libexecdir}/uke-control-qt/application-control
%{_datadir}/polkit-1/actions/org.senemos.uke.resource-control.policy
%{_datadir}/polkit-1/actions/org.senemos.uke.frequency-control.policy
%{_datadir}/polkit-1/actions/org.senemos.uke.qt-kernel-information.policy
%{_datadir}/polkit-1/actions/org.senemos.uke.qt-package-task.policy
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg
%{_datadir}/locale/*/LC_MESSAGES/uke-control-qt.mo
%{_libexecdir}/uke-control-qt/waydroid-installer
%{_datadir}/polkit-1/actions/org.senemos.uke.waydroid-installer.policy
%{_prefix}/lib/systemd/system/uke-waydroid-install@.service

%changelog
* Fri Nov 27 2026 Uke contributors - 1.6.0-22
- Read sensor identities and functions without taking live samples.
- Show the principal device specifications when opening the inventory.

* Fri Nov 27 2026 Uke contributors - 1.6.0-20
- Consolidate battery and USB settings and add local light, display and infrared controls.
- Add a graphical update flow and a cached device inventory.

* Fri Nov 27 2026 Uke contributors - 1.6.0-18
- Send accessibility switch actions through the confirmed flashlight state.
- Align storage monitoring panels and scale the graphs to recorded samples.

* Fri Nov 27 2026 Uke contributors - 1.6.0-16
- Group daily settings on the overview and align charging thresholds and battery readings.
- Control the flashlight directly through the active-session service.
- Show negotiated DisplayPort speed and Linux partitions, and stabilize storage plots.
- Share panel spacing and offer a reviewed return to the accepted boot slot.

* Fri Nov 27 2026 Uke contributors - 1.6.0-13
- Highlight key device readings and keep technical details expandable.
- Separate the running and next boot slots and label cached readings.

* Fri Nov 27 2026 Uke contributors - 1.6.0-11
- Use responsive settings cards and native controls for reviewed preferences.
- Explain the available flashlight levels and percentage requests in English and Turkish.
- Use the signed Control SDK during AArch64 builds while Rawhide repairs its TEE dependency.

* Thu Nov 19 2026 Uke contributors - 1.6.0-8
- Ship the existing Waydroid installer with its Qt frontend.
- Keep the versioned capabilities, unit, authorization and native checks.

* Wed Nov 11 2026 Uke contributors - 1.6.0-7
- Keep all native interface checks in the build and retire the runtime test RPM.
- Preserve the Qt interface and Waydroid installer sources and behavior.

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
