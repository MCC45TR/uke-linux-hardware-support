%global debug_package %{nil}
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: uke-control-2.8.1.tar.xz
Source1: uke-control-frontend-contract.tar.xz
Source2: uke-control-published-spec.txt
Source3: input.json
Source4: copr-public-key.asc
Source6: uke-control-rollback.tar.xz
Source5: uke-control-package-update.tar.xz
Source7: uke-control-speech-overlay.tar.gz
Source8: uke-control-speech-parents.sha256
BuildRequires: pkgconfig(libspa-0.2) pipewire pipewire-libs pipewire-utils wireplumber dbus-daemon
ExclusiveArch: aarch64
SourceLicense: MIT AND Apache-2.0 AND GPL-3.0-or-later AND GPL-2.0-only AND (GPL-2.0-only WITH Linux-syscall-note) AND BSD-2-Clause AND BSD-3-Clause AND GPL-2.0-or-later
BuildRequires: dbus-daemon jq lua
BuildRequires: e2fsprogs dosfstools f2fs-tools
BuildRequires: gcc-c++ make pkgconfig(libdrm) pkgconfig(libsystemd) >= 257
BuildRequires: libdnf5-devel >= 5.4.6.0 pkgconfig(rpm)
BuildRequires: pkgconfig(jsoncpp) pkgconfig(blkid) pkgconfig(openssl)
BuildRequires: pkgconfig(libselinux) systemd-rpm-macros
BuildRequires: pkgconfig(ncursesw)
BuildRequires: pkgconfig(vterm)
BuildRequires: pkgconfig(wayland-client) wayland-devel wayland-protocols-devel
BuildRequires: selinux-policy-devel
BuildRequires: xfsprogs-devel
BuildRequires: findutils createrepo_c rpm-sign gnupg2 pkgconfig(fdisk)
%global uke_control_version 2.17.2
Name: uke-control
Version: %{uke_control_version}
Release: 1%{?dist}
License: MIT AND Apache-2.0 AND GPL-3.0-or-later AND GPL-2.0-only AND (GPL-2.0-only WITH Linux-syscall-note) AND BSD-2-Clause AND BSD-3-Clause AND GPL-2.0-or-later

Summary: Native Uke settings backend and command-line client
Provides: uke-control(frontend-v1) = 1
Provides: uke-control(update-status-v1) = 1
Provides: uke-control(update-status-checkpoint-v2) = 1
Provides: uke-control(slot-selection-v1) = 1
Requires: uke-kernel-update(slot-selection-v1) >= 1
Provides: /usr/libexec/uke-control/frontend-request
Requires: uke-control-client%{?_isa} = %{version}-%{release}
Requires: util-linux-core util-linux systemd procps-ng
Requires: coreutils findutils rpm psmisc
Requires: ethtool >= 7.1
Requires: systemd-udev
Requires: dracut >= 111
Requires: f2fs-tools >= 1.16.0
Requires: policycoreutils
Requires: polkit selinux-policy-targeted
Requires: kmod
Requires: dialog e2fsprogs device-mapper dosfstools
Requires: libdnf5 >= 5.4.6.0
Requires: uke-kernel-update >= 1.8.7
Requires: uke-kernel-update(rollback-policy-v1) >= 1
Requires: uke-kernel-update(transaction-v1) >= 1
Requires: (wireplumber >= 0.5.16 if wireplumber)
Recommends: /usr/lib64/spa-0.2/aec/libspa-aec-webrtc.so
Provides: uke-control-tui = %{version}-%{release}
Provides: uke-control-tui%{?_isa} = %{version}-%{release}
Obsoletes: uke-control-tui < %{version}-%{release}
Provides: uke-control(channel-api) = 1
Provides: uke-control(fedora-returns-v1) = 1
Conflicts: uke-plasma-controls < 1.0.0-23

Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd




Requires: uke-control(aarch-64) = %{version}-%{release}
Requires: sysstat drm-utils
BuildRequires: clang compiler-rt libasan libubsan
BuildRequires: libasan libubsan
BuildRequires: bluez-libs-devel
BuildRequires: dbus-daemon systemd-udev pkgconfig(umockdev-1.0)
BuildRequires: gcc-c++ make systemd-rpm-macros
BuildRequires: jq
BuildRequires: rpm-build
BuildRequires: e2fsprogs
BuildRequires: openssl-devel
BuildRequires: pkgconfig(fdisk) pkgconfig(zlib) util-linux
BuildRequires: pkgconfig(libdrm)
BuildRequires: glib2 strace systemd
BuildRequires: pkgconfig(libsystemd) pkgconfig(wayland-client) pkgconfig(wayland-server)
Requires: (libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1)
Requires: kmod bash systemd NetworkManager openssh-server chrony
Requires: util-linux-core coreutils grep
Requires: bluez
Requires: linux-firmware
Requires: (libcamera-ipa if wireplumber)
Requires: (pipewire-plugin-libcamera if wireplumber)
Requires: uke-firmware >= 3.0.304.0
Requires: senemos-uke-linux-kernel-mainline-modules >= 7.2.9-1.53.native53
Requires(pre): rpm systemd coreutils sed
Requires(posttrans): systemd coreutils sed
Provides: uke-usb-gadget-ownership-api = 2
Provides: uke-usb-manual-cdc-api = 2
Provides: uke-boot-integration(native-platform-profile-ready-v1) = 1
Provides: uke-boot-integration(factory-wlan-v1) = 1
Provides: uke-wifi-factory = 1.8.102-1.fc46
Obsoletes: uke-wifi-factory < 1.8.102-1.fc46
Provides: uke-boot-integration(factory-bluetooth-v1) = 1
Provides: uke-bluetooth = 1.8.102-1.fc46
Obsoletes: uke-bluetooth < 1.8.102-1.fc46
Provides: uke-boot-integration(touch-runtime-v1) = 1
Provides: uke-boot-integration(kernel-touch-dispatch-v1) = 1
Provides: uke-boot-integration(kernel-touch-input-v1) = 1
Provides: uke-boot-integration(kernel-touch-input-v2) = 1
Provides: uke-touchscreen = 1.8.102-1.fc46
Obsoletes: uke-touchscreen < 1.8.102-1.fc46
Provides: uke-kernel-update(alternating-checkpoint-v2) = 1
Conflicts: uke-control < 2.8.1-26
Requires: uke-cdsp-pogo-firmware >= 0.2-8
Provides: uke-kernel-update(cdsp-pogo-firmware-v1) = 1
Requires: uke-o82-color-profile >= 0.7.0-1
Provides: uke-kernel-update(o82-private-color-v5) = 1
Provides: uke-kernel-update(seamless-policy-v2) = 1
Provides: uke-kernel-update(current-trial-replacement-v1) = 1
Provides: uke-kernel-update(rollback-policy-v1) = 1
Provides: uke-kernel-update(slot-selection-v1) = 1
Requires: (libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1) openssl-libs rpm
Requires: systemd >= 254
Requires: e2fsprogs
Requires: uke-installation-profile = 1.8.102-1.fc46
Provides: uke-kernel-update(userdata-ab) = 1
Provides: uke-kernel-update(guarded-automatic) = 1
Provides: uke-kernel-update(transaction-v1) = 1
Provides: uke-kernel-update(final-transaction-v2) = 1
Provides: uke-kernel-update(automatic-policy-migration-v1) = 1
Provides: uke-kernel-update(root-filesystem-v2) = 1
Provides: uke-kernel-update(current-slot-restore) = 1
Provides: uke-kernel-update(kernel-touch-client-v1) = 1
Provides: uke-kernel-update(userdata-fallback-v1) = 1
Provides: uke-kernel-update(trial-health-v2) = 1
Provides: uke-kernel-update(touch-status-v2) = 1
Provides: uke-kernel-update(qualified-firmware-cohort-v1) = 1
Provides: uke-kernel-update(inactive-slot-no-image-backup-v1) = 1
Provides: uke-kernel-update(device-local-video-import-v1) = 1
Provides: uke-installation-profile(rtc-counter-offset-v1) = 1
Provides: uke-installation-profile(rtc-secure-offset-v1) = 1
Requires: (libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1) systemd
Requires: uke-boot-integration = 1.8.102-1.fc46
BuildRequires: clang gcc-c++ compiler-rt pkgconf-pkg-config
BuildRequires: pkgconfig(json-c) pkgconfig(lcms2) pkgconfig(openssl)
BuildRequires: pkgconfig(libdisplay-info)
Requires: e2fsprogs kmod
Source9: uke-boot-integration-1.8.102.tar.xz
Source10: 60-uke-filelists.conf
Source11: control-version.hpp
Source12: merge-kernel-spec.cpp
Source13: copr-control-kernel.sh
Source14: KERNEL-CONSOLIDATION.md
Source15: kernel-consolidation.sh
Source16: control-consolidated-spec.txt
Source17: unstable-release.hpp
Source18: runtime733-version.patch
Source19: retained-o82-color-0.7.0.tar.xz
Source20: unstable-release.cpp
BuildRequires: patch
Provides: uke-o82-color-profile = 0.7.0-2%{?dist}
Provides: uke-o82-color-profile%{?_isa} = 0.7.0-2%{?dist}
Obsoletes: uke-o82-color-profile <= 0.7.0-2%{?dist}
Provides: uke-boot-integration = 1.8.102-1%{?dist}
Provides: uke-boot-integration%{?_isa} = 1.8.102-1%{?dist}
Obsoletes: uke-boot-integration <= 1.8.102-1%{?dist}
Provides: uke-installation-profile = 1.8.102-1%{?dist}
Provides: uke-installation-profile%{?_isa} = 1.8.102-1%{?dist}
Obsoletes: uke-installation-profile <= 1.8.102-1%{?dist}
Provides: uke-kernel-update = 1.8.102-1%{?dist}
Provides: uke-kernel-update%{?_isa} = 1.8.102-1%{?dist}
Obsoletes: uke-kernel-update <= 1.8.102-1%{?dist}
Provides: uke-control-client = %{version}-%{release}
Provides: uke-control-client%{?_isa} = %{version}-%{release}
Obsoletes: uke-control-client < %{version}-%{release}
Provides: uke-control-tests = %{version}-%{release}
Provides: uke-control-tests%{?_isa} = %{version}-%{release}
Obsoletes: uke-control-tests < %{version}-%{release}

# UKE_CONTROL_SESSION_BRIDGE
Source21: uke-control-session-bridge.tar.xz
Source22: copr-control-session.sh
Source23: fold-session-spec.sh
BuildRequires: pkgconfig(alsa)
Provides: uke-speaker-orientation = 0.2.0-3%{?dist}
Provides: uke-speaker-orientation%{?_isa} = 0.2.0-3%{?dist}
Provides: uke-control(session-bridge-v1) = 1
Obsoletes: uke-speaker-orientation <= 0.2.0-2%{?dist}
# UKE_CONTROL_BOOT_HEALTH_REPORT
Source24: uke-control-boot-health.tar.xz
Source25: fold-health-spec.sh
Source26: copr-control-health.sh
BuildRequires: git-core
Provides: uke-control(kernel-health-report-v1) = 1
# UKE_CONTROL_CRITICAL_ROLLBACK
Source27: uke-control-critical-rollback.tar.xz
Source28: fold-critical-spec.sh
Source29: copr-control-critical.sh
Provides: uke-control(critical-rollback-v1) = 1
Provides: uke-control(critical-crash-count-v1) = 1
# UKE_CONTROL_BRIGHTNESS_CURVE
Source30: uke-control-brightness-curve.tar.xz
Source31: fold-brightness-spec.sh
Source32: copr-control-brightness.sh
Provides: uke-control(brightness-curve-v1) = 1
# UKE_CONTROL_SPEAKER_RUNTIME
Source33: uke-control-speaker-runtime.tar.xz
Source34: retained-speaker-runtime-0.4-3.tar.gz
Source35: fold-speaker-spec.sh
Source36: copr-control-speaker.sh
Source37: uke-control-session-fixes.tar.xz
Source38: copr-control-session-fixes.sh
Requires: bash curl jq alsa-utils
Provides: uke-speaker-runtime = 0.4-4%{?dist}
Provides: uke-speaker-runtime%{?_isa} = 0.4-4%{?dist}
Provides: uke-speaker-runtime(gain-readiness-v1) = 1
Provides: uke-control(speaker-runtime-v1) = 1
Obsoletes: uke-speaker-runtime <= 0.4-3%{?dist}
# UKE_CONTROL_NFC_CONSOLIDATION
Source39: uke-control-nfc.tar.xz
Source40: fold-nfc-spec.sh
Source41: copr-control-nfc.sh
BuildRequires: acl kernel-headers
Requires: /usr/bin/ir-ctl
%ifarch aarch64
Requires: uke-kernel(nfc-ir-v2) >= 1
%endif
Provides: uke-nfc-ir = 1.1.0-3%{?dist}
Provides: uke-nfc-ir%{?_isa} = 1.1.0-3%{?dist}
Provides: uke-device-access(nfc-ir-v1) = 1
Provides: uke-nfc-tag-client(abi1) = 1
Provides: uke-control(nfc-ir-integration-v1) = 1
Obsoletes: uke-nfc-ir <= 1.1.0-2%{?dist}
# UKE_CONTROL_NATIVE_PROXIMITY_SELECTION
Source42: 89-uke-native-proximity.rules
Source43: NATIVE-PROXIMITY-SELECTION.md
Source44: fold-native-proximity-spec.sh
Source45: copr-control-native-proximity.sh
Source46: native-proximity-source.json
Provides: uke-control(native-proximity-selection-v1) = 1
# UKE_CONTROL_CHARGE_OWNERSHIP
Source47: uke-control-charge.tar.xz
Source48: fold-charge-spec.sh
Source49: copr-control-charge-fold.sh
Source50: charge-fold-source.json
Requires: upower >= 1.91.5
Provides: uke-charge-control = 1.4.1-8%{?dist}
Provides: uke-charge-control%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-charge-control < 1.4.1-8%{?dist}
Provides: uke-control(charge-bridge-v1) = 1
# UKE_CONTROL_CHARGE_WAIT
Source51: uke-control-charge-wait.tar.xz
BuildRequires: gcc-c++ clang llvm make compiler-rt libasan libubsan nodejs
BuildRequires: pkgconfig(libsystemd) >= 257
BuildRequires: pkgconfig(libcrypto) jq
# UKE_CONTROL_BATTERY_OWNERSHIP
Source52: retained-battery-services-1.4.1-7.tar.xz
Source53: uke-control-battery-services.tar.xz
Source54: uke-battery.conf
Provides: uke-battery-observer = 1.4.1-8%{?dist}
Provides: uke-battery-observer%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-battery-observer < 1.4.1-8%{?dist}
Provides: uke-offline-charge = 1.4.1-8%{?dist}
Provides: uke-offline-charge%{?_isa} = 1.4.1-8%{?dist}
Obsoletes: uke-offline-charge < 1.4.1-8%{?dist}
Provides: uke-control(battery-services-v1) = 1
# UKE_CONTROL_ADAPTIVE_OWNERSHIP
Source55: uke-control-adaptive-services.tar.xz
BuildRequires: cmake ninja-build extra-cmake-modules
BuildRequires: qt6-qtbase-devel kf6-kcoreaddons-devel libksysguard-devel lm_sensors-devel
Provides: uke-adaptive-display = 1.0.16-18%{?dist}
Provides: uke-adaptive-display%{?_isa} = 1.0.16-18%{?dist}
Provides: config(uke-adaptive-display) = 1.0.16-18%{?dist}
Obsoletes: uke-adaptive-display < 1.0.16-18%{?dist}
Provides: uke-control(adaptive-integration-v1) = 1
Requires: iio-sensor-proxy(ssc) >= 1
Requires: uke-ssc-fileservice >= 2026.09.21-8
Requires: coreutils grep sed systemd-udev
Requires: ddcutil >= 3.0.2
Requires: libddcutil >= 3.0.2
Requires: systemd >= 256
Requires: (powerdevil >= 6.8.0 if plasma-workspace)
Requires: (kscreen >= 6.8.0 if plasma-workspace)
Requires: (ksystemstats >= 6.8.0 if plasma-workspace)
Requires: (libksysguard >= 6.8.0 if ksystemstats)
# Only the optional project plugin; all its ELF requirements are explicit below.
%global __requires_exclude_from ^(%{_libdir}/qt6/plugins/ksystemstats/libuke_adreno[.]so|%{_libexecdir}/uke-pogo-desktop)$
Requires: (ld-linux-aarch64.so.1()(64bit) if powerdevil)
Requires: (ld-linux-aarch64.so.1(GLIBC_2.17)(64bit) if powerdevil)
Requires: (libQt6Core.so.6()(64bit) if powerdevil)
Requires: (libQt6Core.so.6(Qt_6)(64bit) if powerdevil)
Requires: (libQt6Core.so.6(Qt_6.12)(64bit) if powerdevil)
Requires: (libQt6DBus.so.6()(64bit) if powerdevil)
Requires: (libQt6DBus.so.6(Qt_6)(64bit) if powerdevil)
Requires: (libc.so.6()(64bit) if powerdevil)
Requires: (libc.so.6(GLIBC_2.17)(64bit) if powerdevil)
Requires: (libc.so.6(GLIBC_2.34)(64bit) if powerdevil)
Requires: (libc.so.6(GLIBC_ABI_DT_RELR)(64bit) if powerdevil)
Requires: (libgcc_s.so.1()(64bit) if powerdevil)
Requires: (libgcc_s.so.1(GCC_3.0)(64bit) if powerdevil)
Requires: (libgcc_s.so.1(GCC_3.3.1)(64bit) if powerdevil)
Requires: (libstdc++.so.6()(64bit) if powerdevil)
Requires: (libstdc++.so.6(CXXABI_1.3)(64bit) if powerdevil)
Requires: (libstdc++.so.6(CXXABI_1.3.9)(64bit) if powerdevil)
Requires: (libstdc++.so.6(GLIBCXX_3.4)(64bit) if powerdevil)
Requires: (libstdc++.so.6(GLIBCXX_3.4.21)(64bit) if powerdevil)
Requires: (libstdc++.so.6(GLIBCXX_3.4.26)(64bit) if powerdevil)
Requires: (libstdc++.so.6(GLIBCXX_3.4.32)(64bit) if powerdevil)
Requires: (libstdc++.so.6(GLIBCXX_3.4.9)(64bit) if powerdevil)
Requires: (rtld(GNU_HASH) if powerdevil)
Requires: (ld-linux-aarch64.so.1()(64bit) if ksystemstats)
Requires: (ld-linux-aarch64.so.1(GLIBC_2.17)(64bit) if ksystemstats)
Requires: (libKF6CoreAddons.so.6()(64bit) if ksystemstats)
Requires: (libKSysGuardSystemStats.so.2()(64bit) if ksystemstats)
Requires: (libQt6Core.so.6()(64bit) if ksystemstats)
Requires: (libQt6Core.so.6(Qt_6)(64bit) if ksystemstats)
Requires: (libQt6Core.so.6(Qt_6.12)(64bit) if ksystemstats)
Requires: (libc.so.6()(64bit) if ksystemstats)
Requires: (libc.so.6(GLIBC_2.17)(64bit) if ksystemstats)
Requires: (libc.so.6(GLIBC_ABI_DT_RELR)(64bit) if ksystemstats)
Requires: (libgcc_s.so.1()(64bit) if ksystemstats)
Requires: (libgcc_s.so.1(GCC_3.0)(64bit) if ksystemstats)
Requires: (libgcc_s.so.1(GCC_3.3.1)(64bit) if ksystemstats)
Requires: (libstdc++.so.6()(64bit) if ksystemstats)
Requires: (libstdc++.so.6(CXXABI_1.3)(64bit) if ksystemstats)
Requires: (libstdc++.so.6(CXXABI_1.3.9)(64bit) if ksystemstats)
Requires: (libstdc++.so.6(GLIBCXX_3.4)(64bit) if ksystemstats)
Requires: (libstdc++.so.6(GLIBCXX_3.4.30)(64bit) if ksystemstats)
Requires: (rtld(GNU_HASH) if ksystemstats)
Source56: uke-control-ssc-services.tar.xz
Source57: uke-fastrpc-notify-check-1.tar.xz
Source58: uke-fastrpc-files-1.tar.xz
Source59: uke-fastrpc-session-1.tar.xz
Source60: uke-control-fastrpc-provider.tar.xz
Source61: uke-control-pogo-integration.tar.xz
Source62: uke-control-selinux-integration.tar.xz
Source63: uke-control-closed-cover-health.tar.xz
Source64: uke-control-motion.tar.xz
Source65: original-fedora-motion-policy.rpm
BuildRequires: checkpolicy policycoreutils cpio bzip2
Provides: uke-control(motion1-v1) = 1
Requires(post): /usr/bin/restorecon
Provides: uke-control(closed-cover-health-v1) = 1
Provides: uke-control(selinux-kernel-abi-v1) = 1
Requires: /usr/bin/checkpolicy /usr/bin/gawk /usr/bin/bzip2
Requires: /usr/bin/semodule /usr/bin/selinuxenabled /usr/bin/getenforce /usr/bin/load_policy
BuildRequires: pkgconfig(Qt6Core) pkgconfig(Qt6DBus) openssl-devel patch pkgconfig(libcbor)
Requires(pre): systemd coreutils rpm
Requires(posttrans): systemd coreutils
Requires: uke-cdsp-pogo-firmware >= 0.2-9
Provides: uke-pogo-auth = 0.1-5%{?dist}
Provides: uke-pogo-auth%{?_isa} = 0.1-5%{?dist}
Provides: uke-pogo-desktop = 0.1-6%{?dist}
Provides: uke-pogo-desktop%{?_isa} = 0.1-6%{?dist}
Provides: uke-control(pogo-integration-v1) = 1
Provides: bundled(minkipc) = 1.0.0
Obsoletes: uke-pogo-auth < 0.1-5%{?dist}
Obsoletes: uke-pogo-desktop < 0.1-6%{?dist}
BuildRequires: fedora-gpg-keys libbsd libyaml
Provides: uke-ssc-fileservice = 2026.09.21-33%{?dist}
Provides: uke-ssc-fileservice%{?_isa} = 2026.09.21-33%{?dist}
Provides: uke-ssc-fileservice(oem-defaults-v1) = 1
Provides: uke-control(ssc-integration-v1) = 1
Obsoletes: uke-ssc-fileservice < 2026.09.21-33%{?dist}
Requires: /usr/share/uke-sensors/odm/json.lst
Requires: /usr/share/uke-sensors/sns_reg.conf
Requires: /usr/lib/firmware/qcom/uke/sensors/sns_tppe.so
Requires: /usr/bin/findmnt /usr/bin/grep /usr/bin/flock
Requires: fastrpc >= 1.0.8-1

# UKE_RGB_CHARGE_SOURCE_BEGIN
Source66: rgb-charge-status.patch
Source67: rgb-charge-source.json
Provides: uke-control(rgb-charge-status-v1) = 1
# UKE_RGB_CHARGE_SOURCE_END
# UKE_USB_API_OUTPUT_SOURCE_BEGIN
Source68: usb-api-output.patch
Source69: usb-api-output-source.json
Source70: rpm-api-output.sh
Source71: control20-parent-runtime.rpm
# UKE_USB_API_OUTPUT_SOURCE_END
# UKE_BOOT_IDENTITY22_HEADER_BEGIN
Source72: boot-status-unstable.patch
Source73: boot-status-unstable.sh
# UKE_BOOT_IDENTITY22_HEADER_END
# UKE_SETUP_CDC23_HEADER_BEGIN
Source74: setup-cdc-overlay.tar.xz
Source75: setup-cdc-native-proof.rpm
Source76: setup-cdc-source.json
Provides: uke-control(setup-cdc-v1) = 1
# UKE_SETUP_CDC23_HEADER_END
# UKE_SUPER_BOOT24_HEADER_BEGIN
Source77: super-boot-overlay.tar.xz
Source78: super-boot-source.json
Provides: uke-control(super-root-boot-v1) = 1
# UKE_SUPER_BOOT24_HEADER_END
# UKE_SUPER_SWAP25_HEADER_BEGIN
Source79: super-swap-overlay.tar.xz
Source80: super-swap-source.json
Provides: uke-control(logical-swap-v1) = 1
Provides: uke-control(setup-cdc-reboot-v1) = 1
Requires: android-tools device-mapper
# UKE_SUPER_SWAP25_HEADER_END
# UKE_BOOT_STARTUP26_HEADER_BEGIN
Source81: boot-startup-overlay.tar.xz
Source82: boot-startup-source.json
Provides: uke-control(boot-serialization-v1) = 1
# UKE_BOOT_STARTUP26_HEADER_END
# UKE_FACTORY_INPUTS27_HEADER_BEGIN
Source83: factory-inputs-overlay.tar.xz
Source84: factory-inputs-source.json
BuildRequires: uke-firmware(oem-display-inputs-v1) = 1
Requires: uke-firmware(oem-display-inputs-v1) = 1
Provides: uke-control(factory-update-inputs-v1) = 1
# UKE_FACTORY_INPUTS27_HEADER_END
# UKE_SWAP_READY28_HEADER_BEGIN
Source85: swap-ready-overlay.tar.xz
Source86: swap-ready-source.json
Provides: uke-control(swap-ready-v1) = 1
# UKE_SWAP_READY28_HEADER_END
# UKE_UKECTL29_HEADER_BEGIN
Source87: ukectl29-overlay.tar.xz
# UKE_UKECTL29_HEADER_END
# UKE_PIDFD30_HEADER_BEGIN
Source88: control-pidfd-wait-overlay.tar.xz
# UKE_PIDFD30_HEADER_END
# UKE_USB_RECOVERY31_HEADER_BEGIN
Source89: control-usb-recovery-overlay.tar.xz
Source90: control-charge-bands.patch
Source91: control-charge-bands-pre.sha256
Source92: control-charge-bands-post.sha256
Source93: battery-reported-capacity.patch
Source94: battery-reported-capacity.sha256
Source95: swap-admission-once.patch
Source96: swap-admission-once-pre.sha256
Source97: swap-admission-once-post.sha256
Source98: motion-tui-integration.patch
Source99: boot-image-view.patch
Source100: boot-image-view-before.sha256
Source101: boot-image-view-after.sha256
Source102: uke-managed-firmware-normal735-runtime.tar.xz
BuildRequires: nlohmann-json-devel binutils
BuildRequires: uke-firmware(factory-pogo-mcu-v1) >= 1
Source103: 0001-uke-boot-admit-signed-merged-kernel-layout.patch
Source104: merged-layout-before.sha256
Source105: merged-layout-after.sha256
Source106: 80-uke-charge-protection.rules
Source107: uke-charge-protection.service
Source108: 0108-uke-scan-super-uevents-without-forks.patch
Source109: 0109-uke-query-cdsp-fastrpc-capabilities.patch
Source110: 0110-keep-failed-candidate-out-of-boot-retries.patch
Source111: 0111-select-merged-normal-kernel-cohort.patch
Source112: normal-abi.sh
Source113: 0113-consolidate-super-geometry-queries.patch
Source114: 0114-validate-stable-super-snapshot.patch
Source115: uke-control-root-capacity54.tar.xz
Source116: 0115-probe-swap-signature-once.patch
Source117: uke-kernel-migration-check-7.3.5-18.fc46.aarch64.rpm
Source118: uke-control-boot-charge58.tar.xz
Source119: 0119-avoid-identical-inventory-writes.patch
Source120: inventory-contract.cpp
Source121: verify-inventory-source.sh
Source122: 0122-hold-plymouth-until-native-plasma-scanout.patch
Source123: https://kojipkgs.fedoraproject.org/packages/fastrpc/1.0.8/1.fc46/data/signed/91211fce/aarch64/fastrpc-1.0.8-1.fc46.aarch64.rpm
Source124: https://kojipkgs.fedoraproject.org/packages/fastrpc/1.0.8/1.fc46/data/signed/91211fce/aarch64/fastrpc-devel-1.0.8-1.fc46.aarch64.rpm
Source125: 61-uke-display-ddc.rules
Source126: uke-control-display-cvt.tar.xz
Source127: keep-newer-automatic-kernel.patch
Source128: uke-control-iris-prepare.tar.xz
Source129: uke-control-boot-notify.tar.xz
Source130: uke-control-display-dock.tar.xz
Source131: https://kojipkgs.fedoraproject.org/packages/quic-teec/1.0.2/3.fc46/data/signed/91211fce/aarch64/quic-teec-1.0.2-3.fc46.aarch64.rpm
Source132: https://kojipkgs.fedoraproject.org/packages/quic-teec/1.0.2/3.fc46/data/signed/91211fce/aarch64/quic-teec-devel-1.0.2-3.fc46.aarch64.rpm
Source133: 0133-check-early-firmware-when-staging.patch
Source134: verify-firmware-stage-order.sh
Source135: uke-o82-resolution-continuity.json
Source136: uke-o82-resolution-continuity.js
Source137: 40-uke-serial-reset.conf
Source138: uke-control-suspend-dsc-policy.tar.xz
Source139: 0139-close-owned-acm-tty-before-configfs-teardown.patch
Source140: 0140-check-owned-acm-tty-teardown.sh
Source141: 0141-allow-only-silent-fuser-no-users.patch
BuildRequires: pkgconfig(ext2fs) pkgconfig(mount)
Provides: uke-control(root-capacity-guard-v1) = 1
BuildRequires: perl-interpreter
Provides: uke-control(normal-release-abi-v1) = 1
BuildRequires: systemd-udev
Provides: uke-cdsp-pogo-firmware = 0.2-15.fc46
Provides: uke-cdsp-pogo-firmware%{?_isa} = 0.2-15.fc46
Obsoletes: uke-cdsp-pogo-firmware <= 0.2-14.fc46
Requires: coreutils e2fsprogs grep /usr/bin/systemd-detect-virt
Requires: uke-firmware(factory-pogo-mcu-v1) >= 1
# UKE_USB_RECOVERY31_HEADER_END
%description
Native Uke device control, terminal and client interfaces, with the retained
installation classifier and automatic alternating kernel delivery in one runtime
package. Original DNF triggers the existing admitted updater after transactions.
Persistent preferences, recovery gates and original desktop packages remain.

%package -n uke-control-client-devel
Version: %{uke_control_version}
Release: 1%{?dist}
Summary: C++ SDK for the native Uke Control client
License: MIT AND Apache-2.0
Requires: uke-control-client%{?_isa} = %{version}-%{release}
Provides: uke-control(frontend-sdk-v1) = 1
Provides: /usr/include/uke-control/frontend.hpp

%description -n uke-control-client-devel
Headers and pkg-config metadata for building unprivileged native frontends.

%prep
# Preserve the exact original Fedora FastRPC implementation used by the
# hardware-free fixtures when Rawhide has moved to a different release.
(
reference=%{_builddir}/uke-fastrpc-fedora-reference
qcomtee=%{_builddir}/uke-qcomtee-fedora-sdk
mkdir "$reference" "$reference/root" "$reference/trust" "$qcomtee" "$qcomtee/pkgconfig"
rpmkeys --dbpath "$reference/trust" --import /usr/share/pki/rpm-gpg/RPM-GPG-KEY-fedora-46-primary
sha256sum -c --strict <<'UKE_FASTRPC_RPMS'
099b47aa0601322a6d872fb908f553b220b5899713485e6f9084e8819b28d5bd  %{SOURCE123}
0178e291ddd6b15e9d1b071a61fecd594734fb3aadefd21e45aad1273e28fa4e  %{SOURCE124}
UKE_FASTRPC_RPMS
sha256sum -c --strict <<'UKE_QCOMTEE_RPMS'
5bced7aae02e6369cd8ca34de8f2c580ac92cd602df8d8c36f501745180711be  %{SOURCE131}
119297caaca64b7bf9965ae01878d9d09373b0f44c4fc77965f473b9f96dd40e  %{SOURCE132}
UKE_QCOMTEE_RPMS
for package in %{SOURCE123} %{SOURCE124} %{SOURCE131} %{SOURCE132}; do
    rpmkeys --dbpath "$reference/trust" --checksig -v "$package" > "$reference/verification"
    grep -Fq 'key fingerprint: d924b10d3e810dabdd8b56b596e7e91491211fce: OK' "$reference/verification"
    ! grep -Eq 'NOKEY|BAD|NOT OK' "$reference/verification"
    case "$package" in
        */quic-teec-*) destination=$qcomtee ;;
        *) destination=$reference/root ;;
    esac
    rpm2cpio "$package" | (cd "$destination" && cpio -idm --quiet --no-absolute-filenames)
done
sed -e "s|^libdir=\"%{_libdir}\"$|libdir=\"$qcomtee%{_libdir}\"|" \
    -e "s|^includedir=\"%{_includedir}\"$|includedir=\"$qcomtee%{_includedir}\"|" \
    "$qcomtee%{_libdir}/pkgconfig/qcomtee.pc" > "$qcomtee/pkgconfig/qcomtee.pc"
grep -Fxq "libdir=\"$qcomtee%{_libdir}\"" "$qcomtee/pkgconfig/qcomtee.pc"
grep -Fxq "includedir=\"$qcomtee%{_includedir}\"" "$qcomtee/pkgconfig/qcomtee.pc"
(
    cd "$reference/root" || exit
    find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > "$reference/extracted.sha256"
)
)

# Admit the reviewed source for the existing device-local firmware helper.
cdsp=%{_builddir}/uke-cdsp-normal-firmware
mkdir -p "$cdsp/inputs"
printf '%s  %s\n' 8b5d41eb03f2c2478fb29df67e4fb106081499db0ffeac99cac1539d7446f059 %{SOURCE102} | sha256sum -c --strict
tar -xJf %{SOURCE102} -C "$cdsp/inputs"
(cd "$cdsp/inputs"; sha256sum -c --strict --quiet SHA256SUMS)
motion_overlay=%{_builddir}/uke-control-motion
mkdir "$motion_overlay"
tar -xJf %{SOURCE64} -C "$motion_overlay"
(cd "$motion_overlay";sha256sum -c --strict --quiet SHA256SUMS;node uke-control/packaging/motion-assets.mjs --check)
(
cd "$motion_overlay/uke-control"
printf '%%s  %%s\n' f35623affeefe33823fe1c8903155fb6aeed68f92558ce244a437bfd2679356b packaging/check-motion-integration.sh | sha256sum -c --strict --quiet -
git apply --check --whitespace=error %{SOURCE98}
git apply --whitespace=error %{SOURCE98}
printf '%%s  %%s\n' bc0a23c65c93de2454126f5a3c3d1d1e936e653dfbc070c9bed14071f7155061 packaging/check-motion-integration.sh | sha256sum -c --strict --quiet -
)
selinux_overlay=%{_builddir}/uke-control-selinux-integration
mkdir "$selinux_overlay"
tar -xJf %{SOURCE62} -C "$selinux_overlay"
(cd "$selinux_overlay";sha256sum -c --strict --quiet SHA256SUMS)
pogo_overlay=%{_builddir}/uke-control-pogo-integration
mkdir "$pogo_overlay"
tar -xJf %{SOURCE61} -C "$pogo_overlay"
(cd "$pogo_overlay";sha256sum -c --strict --quiet SHA256SUMS)
mkdir -p "$pogo_overlay/auth/upstream/minkipc" "$pogo_overlay/auth/upstream/quic-teec" "$pogo_overlay/auth/upstream/qcbor"
for library in minkipc quic-teec qcbor;do
 tar -xzf "$pogo_overlay/upstreams/$library.tar.gz" --strip-components=1 -C "$pogo_overlay/auth/upstream/$library"
done
(cd "$pogo_overlay/auth";patch -p1 --fuzz=0 < uke-pogo-auth/patches/0001-qcomtee-refuse-credential-encoding-errors.patch)

fastrpc_overlay=%{_builddir}/uke-control-fastrpc-provider
mkdir "$fastrpc_overlay"
tar -xJf %{SOURCE60} -C "$fastrpc_overlay"
(cd "$fastrpc_overlay";sha256sum -c --strict --quiet SHA256SUMS)
%setup -q -n uke-control-2.8.1
(
mkdir -p "%{_builddir}/uke-control-2.8.1/work/uke-control"
cd '%{_builddir}/uke-control-2.8.1/work/uke-control'
rm -rf 'uke-control-2.8.1'
/usr/bin/rpmuncompress -x '%{_builddir}/uke-control-2.8.1/inputs/uke-control/uke-control-2.8.1.tar.xz'
STATUS=$?
if [ $STATUS -ne 0 ]; then
  exit $STATUS
fi
cd 'uke-control-2.8.1'
/usr/bin/chmod -Rf a+rX,u+w,g-w,o-w .

(
mkdir -p "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control"
cd '%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control'
rm -rf 'uke-control-2.8.1'
/usr/bin/rpmuncompress -x '%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/inputs/uke-control/uke-control-2.8.1.tar.gz'
STATUS=$?
if [ $STATUS -ne 0 ]; then
  exit $STATUS
fi
cd 'uke-control-2.8.1'
/usr/bin/chmod -Rf a+rX,u+w,g-w,o-w .



)


)

mkdir -p %{_builddir}/uke-control-frontend-contract
tar -xJf %{SOURCE1} -C %{_builddir}/uke-control-frontend-contract
mapfile -t package_sources < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/packages.cpp')
test ${#package_sources[@]} = 1
package_root=${package_sources[0]%/src/packages.cpp}
tar -xJf %{SOURCE5} -C "$package_root"
printf '%s\n' 'include tests/package-policy.mk' >> "$package_root/Makefile"
printf '%s\n' 'include tests/update-status.mk' >> "$package_root/Makefile"
mapfile -t rollback_sources < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/main.cpp')
test ${#rollback_sources[@]} = 1
rollback_root=${rollback_sources[0]%/src/main.cpp}
tar -xJf %{SOURCE6} -C "$rollback_root"
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
sha256sum --check --strict %{SOURCE8}
tar -xzf %{SOURCE7}
bash packaging/merge-speech-catalog.sh packaging/microphone-catalog-template.cpp src/client-catalog.cpp
printf '\ninclude packaging/microphone-speech.mk\n' >> Makefile
)

tar -xJf %{SOURCE9} -C %{_builddir}
(
mkdir -p "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration"
cd '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration'
rm -rf 'uke-boot-integration-1.8.102'
/usr/bin/rpmuncompress -x '%{_builddir}/uke-boot-integration-1.8.102/inputs/uke-boot-integration/uke-boot-integration-1.8.102.tar.xz'
STATUS=$?
if [ $STATUS -ne 0 ]; then
  exit $STATUS
fi
cd 'uke-boot-integration-1.8.102'
/usr/bin/chmod -Rf a+rX,u+w,g-w,o-w .

cp touch-runtime/legacy/src/upstream/nt36532-thp/LICENSE THP-LICENSE

)


mkdir -p '%{_builddir}/uke-control-2.8.1/o82-source/'
tar -xJf %{SOURCE19} -C '%{_builddir}/uke-control-2.8.1/o82-source/'

(
mkdir -p '%{_builddir}/uke-control-2.8.1/o82-build/'
cd '%{_builddir}/uke-control-2.8.1/o82-build/'
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-color-profile.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/O82-COLOR-PROFILE.md .
cp %{_builddir}/uke-control-2.8.1/o82-source/LICENSE .
cp %{_builddir}/uke-control-2.8.1/o82-source/uke-o82-hdr-format.h .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-hdr-edid-contract.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-regdma.h .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-color-firmware.h .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-gamut-grid-contract.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-color-before-grid.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-sixzone-format.h %{_builddir}/uke-control-2.8.1/o82-source/o82-sixzone.h %{_builddir}/uke-control-2.8.1/o82-source/o82-sixzone-payload-contract.cpp %{_builddir}/uke-control-2.8.1/o82-source/o82-sixzone-contract.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-hsic.h %{_builddir}/uke-control-2.8.1/o82-source/o82-hsic-format.h %{_builddir}/uke-control-2.8.1/o82-source/o82-hsic-payload-contract.cpp %{_builddir}/uke-control-2.8.1/o82-source/o82-hsic-contract.cpp .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-memory-format.h %{_builddir}/uke-control-2.8.1/o82-source/o82-memory-color.h .
cp %{_builddir}/uke-control-2.8.1/o82-source/GPL-2.0-only.txt .
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-stock-profile.hpp %{_builddir}/uke-control-2.8.1/o82-source/o82-stock-profile-contract.cpp .
mkdir -p stock-profile
cp %{_builddir}/uke-control-2.8.1/o82-source/local.hpp %{_builddir}/uke-control-2.8.1/o82-source/oem-video.hpp stock-profile/
mkdir -p src/display tests
cp %{_builddir}/uke-control-2.8.1/o82-source/uke-o82-hdr-format.h %{_builddir}/uke-control-2.8.1/o82-source/o82-color-firmware.h %{_builddir}/uke-control-2.8.1/o82-source/dpu_o82_profiles.c %{_builddir}/uke-control-2.8.1/o82-source/dpu_o82_profiles.h src/display/
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-sixzone-format.h %{_builddir}/uke-control-2.8.1/o82-source/o82-hsic-format.h src/display/
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-memory-format.h src/display/
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-profile-loader-contract.c %{_builddir}/uke-control-2.8.1/o82-source/o82-profile-loader-contract.sh tests/
cp %{_builddir}/uke-control-2.8.1/o82-source/o82-memory-payload-contract.cpp %{_builddir}/uke-control-2.8.1/o82-source/o82-memory-payload-contract.sh tests/
cp %{_builddir}/uke-control-2.8.1/o82-source/dpu_o82_lutbus.c %{_builddir}/uke-control-2.8.1/o82-source/dpu_o82_lutbus.h src/display/

)

mapfile -t version_headers < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/version.hpp')
test ${#version_headers[@]} = 1
grep -Fxq 'inline constexpr const char* application_version="%{uke_control_version}";' %{SOURCE11}
install -m644 %{SOURCE11} "${version_headers[0]}"
install -m644 %{SOURCE14} KERNEL-CONSOLIDATION.md
patch_stage=$(mktemp -d)
mkdir -p "$patch_stage/uke-boot/src" "$patch_stage/uke-boot/tests"
boot_root=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
cp "$boot_root/src/o82-private-color.hpp" "$patch_stage/uke-boot/src/"
cp "$boot_root/tests/o82-private-color.cpp" "$patch_stage/uke-boot/tests/"
cp "$boot_root/tests/managed-firmware.sh" "$patch_stage/uke-boot/tests/"
cp '%{_builddir}/uke-control-2.8.1/o82-build/o82-stock-profile.hpp' '%{_builddir}/uke-control-2.8.1/o82-build/o82-stock-profile-contract.cpp' "$patch_stage/"
patch --batch --fuzz=0 --dry-run -p1 -d "$patch_stage" < %{SOURCE18}
patch --batch --fuzz=0 -p1 -d "$patch_stage" < %{SOURCE18}
install -m644 "$patch_stage/uke-boot/src/o82-private-color.hpp" "$boot_root/src/"
install -m644 "$patch_stage/uke-boot/tests/o82-private-color.cpp" "$boot_root/tests/"
install -m644 "$patch_stage/uke-boot/tests/managed-firmware.sh" "$boot_root/tests/"
install -m644 "$patch_stage/o82-stock-profile.hpp" "$patch_stage/o82-stock-profile-contract.cpp" '%{_builddir}/uke-control-2.8.1/o82-build/'
install -m644 %{SOURCE17} "$boot_root/src/unstable-release.hpp"
install -m644 %{SOURCE17} '%{_builddir}/uke-control-2.8.1/o82-build/unstable-release.hpp'
find "$patch_stage" -depth -delete
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/LICENSE' 'BOOT-RUNTIME-LICENSE'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/LICENSE.GPL-3.0' 'BOOT-RUNTIME-LICENSE.GPL-3.0'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/THP-LICENSE' 'BOOT-RUNTIME-THP-LICENSE'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/README.md' 'BOOT-RUNTIME-README.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/BLUETOOTH.md' 'BOOT-RUNTIME-BLUETOOTH.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/USB-TYPEC.md' 'BOOT-RUNTIME-USB-TYPEC.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/NATIVE-POWER-PROFILES.md' 'BOOT-RUNTIME-NATIVE-POWER-PROFILES.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/TOUCH-RUNTIME.md' 'BOOT-RUNTIME-TOUCH-RUNTIME.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/SHUTDOWN-TRACE.md' 'BOOT-RUNTIME-SHUTDOWN-TRACE.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/KERNEL-UPDATES.md' 'BOOT-RUNTIME-KERNEL-UPDATES.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/USERDATA-AB.md' 'BOOT-RUNTIME-USERDATA-AB.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/RTC-PERSISTENCE.md' 'BOOT-RUNTIME-RTC-PERSISTENCE.md'
cp '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/docs/MANAGED-FIRMWARE.md' 'BOOT-RUNTIME-MANAGED-FIRMWARE.md'
cp '%{_builddir}/uke-control-2.8.1/o82-build/LICENSE' O82-COLOR-LICENSE
cp '%{_builddir}/uke-control-2.8.1/o82-build/O82-COLOR-PROFILE.md' O82-COLOR-PROFILE.md

mkdir -p %{_builddir}/uke-control-session
tar -xJf %{SOURCE21} -C %{_builddir}/uke-control-session
mkdir -p %{_builddir}/uke-control-boot-health
tar -xJf %{SOURCE24} -C %{_builddir}/uke-control-boot-health
bash %{_builddir}/uke-control-boot-health/apply-boot-health-report.sh "$boot_root"
mkdir -p %{_builddir}/uke-control-critical-rollback
tar -xJf %{SOURCE27} -C %{_builddir}/uke-control-critical-rollback
mapfile -t critical_status < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/update-status.cpp')
test ${#critical_status[@]} = 1
bash %{_builddir}/uke-control-critical-rollback/apply-critical-rollback.sh "$boot_root" "${critical_status[0]}"
mkdir -p %{_builddir}/uke-control-brightness
tar -xJf %{SOURCE30} -C %{_builddir}/uke-control-brightness
mapfile -t curve_main < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/main.cpp')
test ${#curve_main[@]} = 1
curve_control=${curve_main[0]%/src/main.cpp}
printf 'a0301229ab438127f1c88f496fded02f93293366a3b6fc01a7f6abe47c7d9abd  %s/src/main.cpp
e629dbf9e1cc198e22bf3d8ee17745b3b9ac221686e947bd2b398c1ff2b4b1cc  %s/src/cli.cpp
' "$curve_control" "$curve_control" | sha256sum -c --strict --quiet -
git -C %{_builddir}/uke-control-brightness init -q
git -C "$curve_control" --git-dir %{_builddir}/uke-control-brightness/.git --work-tree "$curve_control" apply -p2 --check --whitespace=error %{_builddir}/uke-control-brightness/packaging/brightness-curve-cli.patch
git -C "$curve_control" --git-dir %{_builddir}/uke-control-brightness/.git --work-tree "$curve_control" apply -p2 --whitespace=error %{_builddir}/uke-control-brightness/packaging/brightness-curve-cli.patch
mkdir -p %{_builddir}/uke-control-speaker-overlay %{_builddir}/uke-control-speaker
tar -xJf %{SOURCE33} -C %{_builddir}/uke-control-speaker-overlay
tar -xzf %{SOURCE34} -C %{_builddir}/uke-control-speaker
git -C %{_builddir}/uke-control-speaker init -q
git -C %{_builddir}/uke-control-speaker apply --check --whitespace=error %{_builddir}/uke-control-speaker-overlay/packaging/speaker-runtime.patch
git -C %{_builddir}/uke-control-speaker apply --whitespace=error %{_builddir}/uke-control-speaker-overlay/packaging/speaker-runtime.patch
install -m644 %{_builddir}/uke-control-speaker-overlay/packaging/SPEAKER-RUNTIME-GPL-2.0 SPEAKER-RUNTIME-GPL-2.0
install -m644 %{_builddir}/uke-control-speaker/LICENSE SPEAKER-RUNTIME-ORIGINAL-LICENSE
mkdir -p %{_builddir}/uke-control-session-fixes
tar -xJf %{SOURCE37} -C %{_builddir}/uke-control-session-fixes
bash %{_builddir}/uke-control-session-fixes/packaging/apply-session-fixes.sh "$boot_root" %{_builddir}/uke-control-brightness
mkdir -p %{_builddir}/uke-control-nfc
tar -xJf %{SOURCE39} -C %{_builddir}/uke-control-nfc
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSE NFC-MIT-LICENSE
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSES/GPL-2.0-only.txt NFC-GPL-2.0-only.txt
install -m644 %{_builddir}/uke-control-nfc/nfc/LICENSES/Linux-syscall-note NFC-Linux-syscall-note
mkdir -p %{_builddir}/uke-control-charge
tar -xJf %{SOURCE47} -C %{_builddir}/uke-control-charge
bash %{_builddir}/uke-control-charge/packaging/apply-charge-overlay.sh "$curve_control" %{_builddir}/uke-control-charge
install -m644 %{_builddir}/uke-control-charge/CHARGE-CONSOLIDATION.md CHARGE-CONSOLIDATION.md
mkdir -p %{_builddir}/uke-control-charge-wait
tar -xJf %{SOURCE51} -C %{_builddir}/uke-control-charge-wait
bash %{_builddir}/uke-control-charge-wait/apply-charge-wait.sh %{_builddir}/uke-control-charge/runtime %{_builddir}/uke-control-charge-wait
install -m644 %{_builddir}/uke-control-charge-wait/CHARGE-WAIT.md CHARGE-WAIT.md
mkdir -p %{_builddir}/uke-control-battery/runtime %{_builddir}/uke-control-battery/overlay
tar -xJf %{SOURCE52} --strip-components=1 -C %{_builddir}/uke-control-battery/runtime
(cd %{_builddir}/uke-control-battery/runtime;sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
(
cd %{_builddir}/uke-control-battery/runtime
git apply --check --whitespace=error %{SOURCE93}
git apply --whitespace=error %{SOURCE93}
sha256sum -c --strict --quiet %{SOURCE94}
)
tar -xJf %{SOURCE53} -C %{_builddir}/uke-control-battery/overlay
(cd %{_builddir}/uke-control-battery/overlay;sha256sum -c --strict --quiet SHA256SUMS)
install -m644 %{_builddir}/uke-control-battery/overlay/BATTERY-SERVICES.md BATTERY-SERVICES.md
mkdir -p %{_builddir}/uke-control-adaptive-overlay
tar -xJf %{SOURCE55} -C %{_builddir}/uke-control-adaptive-overlay
(cd %{_builddir}/uke-control-adaptive-overlay;sha256sum -c --strict --quiet SHA256SUMS)
bash %{_builddir}/uke-control-adaptive-overlay/prepare-adaptive-sources.sh %{_builddir}/uke-control-2.8.1 %{_builddir}/uke-control-adaptive %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json
install -m644 %{SOURCE125} %{_builddir}/uke-control-adaptive/runtime/packaging/61-uke-display-ddc.rules
install -m644 %{_builddir}/uke-control-adaptive-overlay/ADAPTIVE-SERVICES.md ADAPTIVE-SERVICES.md
mkdir -p %{_builddir}/uke-control-ssc-overlay
tar -xJf %{SOURCE56} -C %{_builddir}/uke-control-ssc-overlay
(cd %{_builddir}/uke-control-ssc-overlay;sha256sum -c --strict --quiet SHA256SUMS)
ssc=%{_builddir}/uke-control-ssc-overlay
mkdir "$ssc/legacy"
tar -xzf "$ssc/inputs/hexagonrpc-510c87e5.tar.gz" -C "$ssc/legacy" --strip-components=1
(cd "$ssc/legacy";for change in ../inputs/000*.patch;do patch -p1 < "$change";done)
mkdir "$ssc/qualified" "$ssc/qualified/files" "$ssc/qualified/session"
tar -xJf %{SOURCE57} -C "$ssc/qualified" --strip-components=1
tar -xJf %{SOURCE58} -C "$ssc/qualified/files" --strip-components=1
tar -xJf %{SOURCE59} -C "$ssc/qualified/session" --strip-components=1
for directory in "$ssc/qualified" "$ssc/qualified/files" "$ssc/qualified/session";do
 (cd "$directory";sha256sum -c --strict --quiet SHA256SUMS)
done
install -m644 "$ssc/SSC-SERVICES.md" SSC-SERVICES.md

closed_cover=%{_builddir}/uke-control-closed-cover-health
mkdir "$closed_cover"
tar -xJf %{SOURCE63} -C "$closed_cover"
(cd "$closed_cover";sha256sum -c --strict --quiet SHA256SUMS)
printf '%%s  %%s\n' '4bdb285fddd5ce58bb2264640582a03260cda328a0dbe9baf403b24d80e71cb7' '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/src/userdata-update.hpp' | sha256sum -c --strict --quiet -
bash "$closed_cover/uke-control/packaging/apply-closed-cover-health.sh" '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102' "$closed_cover/uke-boot"
printf '%%s  %%s\n' '087a01db8a0e6981feea3a874dbe941601c6bc373ce4e27db294470b686d27b0' '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/src/userdata-update.hpp' | sha256sum -c --strict --quiet -

# UKE_RGB_CHARGE_PREP_BEGIN
(
rgb_root="%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
cd "$rgb_root"
jq -r '.files[]|.before_sha256+"  "+.path' %{SOURCE67} | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .patch_sha256 %{SOURCE67})" %{SOURCE66} | sha256sum -c --strict --quiet -
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE66}
patch -p1 --fuzz=0 --forward -i %{SOURCE66}
jq -r '.files[]|.after_sha256+"  "+.path' %{SOURCE67} | sha256sum -c --strict --quiet -
)
# UKE_RGB_CHARGE_PREP_END
# UKE_USB_API_OUTPUT_PREP_BEGIN
(
usb_root="%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
usb_before=$(mktemp -d)
trap 'find "$usb_before" -depth -delete' EXIT
cd "$usb_root"
cp --parents src/backend.cpp src/usb-request.cpp "$usb_before/"
printf '%s  %s\n' "$(jq -er .patch_sha256 %{SOURCE69})" %{SOURCE68} | sha256sum -c --strict --quiet -
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE68}
patch -p1 --fuzz=0 --forward -i %{SOURCE68}
mkdir "$usb_before/inverse"
cp --parents src/backend.cpp src/usb-request.cpp "$usb_before/inverse/"
(cd "$usb_before/inverse";patch -p1 --fuzz=0 --reverse -i %{SOURCE68})
cmp "$usb_before/src/backend.cpp" "$usb_before/inverse/src/backend.cpp"
cmp "$usb_before/src/usb-request.cpp" "$usb_before/inverse/src/usb-request.cpp"
)
# UKE_USB_API_OUTPUT_PREP_END
# UKE_BOOT_IDENTITY22_PREP_BEGIN
(
cd "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102"
printf 'e0dced833f7f93624df04a7519eae34a3becb97fdd022906796b609df6bcef71  src/boot-status.cpp\n60bf5d118bc5fc88629358118aafd3aa17dc98824ce3589f1214c94a0d719192  src/unstable-release.hpp\n' | sha256sum -c --strict --quiet -
boot_before=$(mktemp -d)
trap 'find "$boot_before" -depth -delete' EXIT
cp --parents src/boot-status.cpp tests/boot-status.sh "$boot_before/"
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE72}
patch -p1 --fuzz=0 --forward -i %{SOURCE72}
printf '92089693d0373683a64238c88be8989588faf0909a02c1737963d392d206e394  src/boot-status.cpp\n' | sha256sum -c --strict --quiet -
mkdir "$boot_before/inverse"
cp --parents src/boot-status.cpp tests/boot-status.sh "$boot_before/inverse/"
(cd "$boot_before/inverse";patch -p1 --fuzz=0 --reverse -i %{SOURCE72})
cmp "$boot_before/src/boot-status.cpp" "$boot_before/inverse/src/boot-status.cpp"
cmp "$boot_before/tests/boot-status.sh" "$boot_before/inverse/tests/boot-status.sh"
)
# UKE_BOOT_IDENTITY22_PREP_END
# UKE_SETUP_CDC23_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-setup-cdc
mkdir "$overlay"
tar -xJf %{SOURCE74} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
printf '%s  %s\n' "$(jq -er .native_proof.sha256 %{SOURCE76})" %{SOURCE75} | sha256sum -c --strict --quiet -
mkdir "$overlay/proof"
rpm2cpio %{SOURCE75} | (cd "$overlay/proof";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
proof="$overlay/proof/usr/share/uke-control-setup-source-check"
test "$(grep -c '^PASS: actual setup policy' "$proof/native-results.txt")" = 4
(cd "$overlay";awk '$2~/^(src\/setup-cdc\.(cpp|hpp)|tests\/setup-cdc\.(cpp|sh))$/' "$proof/qualified-source.sha256" > qualified-new-source.sha256;test "$(wc -l < qualified-new-source.sha256)" = 4;sha256sum -c --strict --quiet qualified-new-source.sha256)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/packaging/apply-setup-cdc.sh" "$control" "$overlay"
)
# UKE_SETUP_CDC23_PREP_END
# UKE_SUPER_BOOT24_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-super-boot
mkdir "$overlay"
tar -xJf %{SOURCE77} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
bash "$overlay/apply.sh" "$boot" "$overlay"
)
# UKE_SUPER_BOOT24_PREP_END
# UKE_SUPER_SWAP25_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-super-swap
mkdir "$overlay"
tar -xJf %{SOURCE79} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_SUPER_SWAP25_PREP_END
# UKE_BOOT_STARTUP26_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-boot-startup
mkdir "$overlay"
tar -xJf %{SOURCE81} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_BOOT_STARTUP26_PREP_END
# UKE_FACTORY_INPUTS27_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-factory-inputs
mkdir "$overlay"
tar -xJf %{SOURCE83} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
color=%{_builddir}/uke-control-2.8.1/o82-build
bash "$overlay/apply.sh" "$boot" "$color" "$overlay"
)
# UKE_FACTORY_INPUTS27_PREP_END
# UKE_SWAP_READY28_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-swap-ready
mkdir "$overlay"
tar -xJf %{SOURCE85} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_SWAP_READY28_PREP_END
# UKE_UKECTL29_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-ukectl29
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
mkdir "$overlay"
tar -xJf %{SOURCE87} -C "$overlay"
(cd "$overlay"; sha256sum --check --strict --quiet SHA256SUMS)
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_UKECTL29_PREP_END
# UKE_ROOT_CAPACITY54_PREP_BEGIN
(
set -Eeuo pipefail
root_capacity=%{_builddir}/uke-control-root-capacity54
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
printf '%%s  %%s\n' 6250a0775b8c23f40a5c042cf3f203a8f853af85d388b74cdd8e69131318e232 %{SOURCE115} | sha256sum -c --strict --quiet -
mkdir "$root_capacity"
tar -xJf %{SOURCE115} -C "$root_capacity"
(cd "$root_capacity"; sha256sum -c --strict --quiet SHA256SUMS)
(cd "$control"; sha256sum -c --strict --quiet "$root_capacity/parent.sha256")
install -m644 "$root_capacity/ukectl.cpp" "$root_capacity/root-capacity.hpp" "$control/src/"
install -m644 "$root_capacity/root-capacity.sh" "$control/tests/root-capacity.sh"
)
# UKE_ROOT_CAPACITY54_PREP_END
# UKE_PIDFD30_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-pidfd30
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
mkdir "$overlay"
tar -xJf %{SOURCE88} -C "$overlay"
(cd "$overlay"; sha256sum --check --strict --quiet SHA256SUMS)
cd "$control"
printf '%%s  %%s\n' d8b826c3436634509f8eff4f174a87d18ed57b7469725ee88e1cdc04c6a82d16 src/backend.cpp | sha256sum -c --strict
cp src/backend.cpp "$overlay/original.cpp"
patch -p1 --fuzz=0 --forward -i "$overlay/pidfd-wait.patch"
printf '%%s  %%s\n' f246cc5b94ecc22a145f572811f4420a47b9b420b00ac020562f5697e13f997c src/backend.cpp | sha256sum -c --strict
cp src/backend.cpp "$overlay/rollback.cpp"
patch --reverse --fuzz=0 "$overlay/rollback.cpp" -i "$overlay/pidfd-wait.patch"
cmp "$overlay/original.cpp" "$overlay/rollback.cpp"
install -m644 "$overlay/tests/execute.cpp" "$overlay/tests/execute-pidfd.sh" tests/
)
# UKE_PIDFD30_PREP_END
# UKE_USB_RECOVERY31_PREP_BEGIN
(
overlay=%{_builddir}/uke-usb-recovery31
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
mkdir "$overlay"
tar -xJf %{SOURCE89} -C "$overlay"
cd "$overlay"
printf '%%s  %%s\n' bae96b3821b7fb9e1274d7e69d9615d6bf1567e155adc14df6491f00871ce967 qualification.rpm | sha256sum -c --strict
rpm2cpio qualification.rpm | cpio -i --quiet --to-stdout './usr/share/uke-usb-recovery-source-check/SHA256SUMS' > qualified.sha256
sha256sum --check --strict --quiet qualified.sha256

cd "$boot"
printf '%%s\n' \
    '83162bae7929c921c71c630d81c787d5ac866d1916662c21a86ce3783d9129b9  src/uke-usb-network.sh' \
    '2ff2df33b27b740b50a2cfece92497a802960140781d0cf8f9a0175ba8cc789f  tests/usb-auto.sh' \
    '0830308aab2435bcb93c8cec9e15dd2ca9a3b0dfd87e8d16585cd5e135dca1e7  tests/usb-ownership.sh' | sha256sum -c --strict
install -m755 "$overlay/src/uke-usb-network.sh" src/uke-usb-network.sh
install -m644 "$overlay/tests/usb-auto.sh" "$overlay/tests/usb-ownership.sh" tests/
)
# UKE_USB_RECOVERY31_PREP_END
# UKE_ACM_LOCAL_TTY_TEARDOWN_BEGIN
(
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
cd "$boot"
printf '%%s\n' 'e8d6eca75552e0985eaade475a9e56aa9a7d4b4c8fa237aa3703eb68598c2cf7  src/uke-usb-network.sh' | sha256sum -c --strict
patch -p1 --batch --fuzz=0 --forward -i %{SOURCE139}
printf '%%s\n' 'fc5230f2b2d910198ebdf6ce84f45b9b080bc7f3f96adceffec5d925c65ec036  src/uke-usb-network.sh' | sha256sum -c --strict
patch -p1 --batch --fuzz=0 --forward -i %{SOURCE141}
printf '%%s\n' 'f8817b9774881f569ab1b5c6b0a209d0e0fd24c6a1927b1a6dffa3d23cb31b80  src/uke-usb-network.sh' | sha256sum -c --strict
bash %{SOURCE140} "$PWD/src/uke-usb-network.sh"
)
# UKE_ACM_LOCAL_TTY_TEARDOWN_END
(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
cd "$control"
sha256sum --check --strict --quiet %{SOURCE91}
patch -p1 --batch --fuzz=0 --forward -i %{SOURCE90}
sha256sum --check --strict --quiet %{SOURCE92}
)

(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
overlay=%{_builddir}/uke-control-swap-ready
cd "$control"
install -m644 "$overlay/tests/swap-ready.sh" tests/swap-ready.sh
sha256sum -c --strict --quiet %{SOURCE96}
git apply --check --whitespace=error %{SOURCE95}
git apply --whitespace=error %{SOURCE95}
sha256sum -c --strict --quiet %{SOURCE97}
install -m644 tests/swap-ready.sh "$overlay/tests/swap-ready.sh"
)

(
inventory=%{_builddir}/uke-control-inventory61
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
mkdir "$inventory"
cd "$control"
printf '%%s  %%s\n' \
    0c0050328b79a57c3de337654fd95e67b87c1eae87d25d2939205b4b855eac6b src/backend.cpp \
    06767ad4cd4a0cb57af42f5dede50dfca2378a5e9230567a1ad742d2287b82a9 %{SOURCE119} \
    4202d0ebe290fb6048f6b97bf571d18ce797a94148c7cc586672f4607227c0ae %{SOURCE120} \
    d601a242d3448fe90686d2025abedb3d404c4814f5c1005ce11b0be1d3044c3a %{SOURCE121} | sha256sum -c --strict
cp src/backend.cpp "$inventory/original.cpp"
git apply --check --whitespace=error %{SOURCE119}
git apply --whitespace=error %{SOURCE119}
printf '%%s  %%s\n' 353e97180d17689d6a0c4602479347cd14a6d777269b11cc22ab5480bd0251a2 src/backend.cpp | sha256sum -c --strict
git apply --reverse --check %{SOURCE119}
)



# Keep the signed logical-swap archive intact before applying this scan change.
(
cd %{_builddir}/uke-control-super-swap/runtime
printf '%s  %s\n' c3e5b06fc7c0279d7f8193142524b4fe8f3b2f6ba6ef221cf2fa65bc01183faa select-uke-super.sh f7befbd38fb8d4cc95bba0761c0010019e7a6697fc6f09ddff994984aa4c477f %{SOURCE108} | sha256sum -c --strict --quiet -
patch --batch --fuzz=0 --forward -p1 -i %{SOURCE108}
printf '%s  %s\n' 485bb45445c94278dd46ad61e81822ccfaf5cc71f5913ab320886bacf8fa34be select-uke-super.sh | sha256sum -c --strict --quiet -
bash -n select-uke-super.sh
patch --batch --fuzz=0 --reverse --dry-run -p1 -i %{SOURCE108}
)

(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
git apply --check --whitespace=error %{SOURCE109}
git apply --whitespace=error %{SOURCE109}
git apply --reverse --check %{SOURCE109}
)

(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
git apply --check --whitespace=error %{SOURCE111}
git apply --whitespace=error %{SOURCE111}
git apply --reverse --check %{SOURCE111}
)

(
cd %{_builddir}/uke-control-super-swap/runtime
printf '%%s  %%s\n' bba8fb5baec86e722dde1f9381e281137b22ef561e06e022e12ef333366f877c inspect-fastboot-super.sh ffc6a46c3cb6269a35d877ca1905e8f0757698ff54fe4baee3a2e88ee823c15a map-fastboot-swap.sh a91f3894097ef032b4e2c239665155d3a75972d4587a29c3dc5a5fb3d9decbc9 %{SOURCE113} | sha256sum -c --strict --quiet -
patch --batch --fuzz=0 --forward --dry-run -p1 -i %{SOURCE113}
patch --batch --fuzz=0 --forward -p1 -i %{SOURCE113}
printf '%%s  %%s\n' dc66f034081b1ffd6455762986df1f1cccfcf222e8bb7517673368320723280b inspect-fastboot-super.sh c91392a01b5e84c19e08fd14484f797c86ff137e03ef04707916205bca02f88c map-fastboot-swap.sh | sha256sum -c --strict --quiet -
bash -n inspect-fastboot-super.sh
bash -n map-fastboot-swap.sh
patch --batch --fuzz=0 --reverse --dry-run -p1 -i %{SOURCE113}
)

# Preserve all earlier signed source layers before replacing the second liblp parse.
(
cd %{_builddir}/uke-control-super-swap/runtime
printf '%%s  %%s\n' c91392a01b5e84c19e08fd14484f797c86ff137e03ef04707916205bca02f88c map-fastboot-swap.sh 684a2e4f92d085446a13413ee513fbd672260aab1ff71c2c7b9fd1d8bf1b2a62 %{SOURCE114} | sha256sum -c --strict --quiet -
patch --batch --fuzz=0 --forward --dry-run -p1 -i %{SOURCE114}
patch --batch --fuzz=0 --forward -p1 -i %{SOURCE114}
printf '%%s  %%s\n' 2683f31a971aba0d39fbe626486f2e001d73e918806af18684c7983493446dd4 map-fastboot-swap.sh | sha256sum -c --strict --quiet -
bash -n map-fastboot-swap.sh
patch --batch --fuzz=0 --reverse --dry-run -p1 -i %{SOURCE114}
)

# Match the signed source-qualified single swap probe.
(
cd %{_builddir}/uke-control-super-swap/runtime
printf '%%s  %%s\n' 97bfbec189c8b72b5af53a4f183c790814005e152b3f3695eb7ac2f44b42342e %{SOURCE117} | sha256sum -c --strict -
receipt=$(rpm2cpio %{SOURCE117} | cpio -i --quiet --to-stdout ./usr/share/uke-kernel-migration-check/swap-signature-once/qualification.txt)
grep -Fxq 'Real blkid accepts the expected swap signature, rejects wrong/unformatted images, and the actual shell block rejects all malformed replies.' <<< "$receipt"
printf '%%s  %%s\n' 2683f31a971aba0d39fbe626486f2e001d73e918806af18684c7983493446dd4 map-fastboot-swap.sh 07fe289ef11838254fef47c810c7df2cd4a5047a81bb0ae53588766c1aee6818 %{SOURCE116} | sha256sum -c --strict -
patch --batch --fuzz=0 --forward --dry-run -p1 -i %{SOURCE116}
patch --batch --fuzz=0 --forward -p1 -i %{SOURCE116}
printf '%%s  %%s\n' 051c5cbbf729d93a7aa994cd07e275953b0471f4021e7689ad440d1ab72c09f6 map-fastboot-swap.sh | sha256sum -c --strict -
bash -n map-fastboot-swap.sh
patch --batch --fuzz=0 --reverse --dry-run -p1 -i %{SOURCE116}
)


(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
cd "$control"
sha256sum -c --strict <<'UKE_HANDOFF_BEFORE'
2bc08fb4a2482435d7969f8a5def1e3fd9775828b32ecb53b41a02b21a9c10a5  Makefile
6c3fd034e890ad7b48655cff4386d82ff99bf9124a4bc4e61499639eb44dae43  src/main.cpp
1a6877e3ab774f4c925373fed1802907171f5c402fc6ba4199bc1b9f1c7143e1  %{SOURCE122}
UKE_HANDOFF_BEFORE
git apply --check --whitespace=error %{SOURCE122}
git apply --whitespace=error %{SOURCE122}
sha256sum -c --strict <<'UKE_HANDOFF_AFTER'
ef8b526e262cab8d4a4427834d15901f1f8aaaf2378d8663fb0379531d3fd039  Makefile
46300e3b6c33f826147be714c0e7850186974935f3a226a0bac6aaf16b43d06d  src/main.cpp
0d1c5d752f35379eb0d0257c716dad9c464efd4962e69138269bab92f705cf51  src/plymouth-handoff.cpp
01ae6d0bd4657a2bc15a9153c79938ea56ea311e47e72c5524258415091880f7  src/plymouth-handoff.hpp
0c80b830ba5c8d3d078b4b4c182528bd036da8e3452ad7558df9137b9556849c  src/plymouth-handoff-sequence.hpp
cfdaae32fef68923b6ce59f2a9e7acb4f8ece3fb10bf9d1901253e4615bf88e3  tests/plymouth-handoff.cpp
e0690cebc430bfe9c354b8e333c9969e2c2b9900ec3f865f1b6345c90f031d7d  packaging/50-uke-display-handoff.conf
1afcc69f62c7e74123ec54f080896902df9b3d9b6cc0961a79a800371221dd7d  packaging/50-uke-display-handoff-wait.conf
UKE_HANDOFF_AFTER
git apply --reverse --check %{SOURCE122}
)

(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
cvt_overlay=%{_builddir}/uke-control-display-cvt
echo 6dcf9e7fc571428f6ab2dae9b47e3e897094978fca6147aeeabd246babf61d63  "%{SOURCE126}" | sha256sum -c --strict
mkdir "$cvt_overlay"
tar -xJf %{SOURCE126} -C "$cvt_overlay"
(cd "$cvt_overlay"; sha256sum -c --strict SHA256SUMS)
bash "$cvt_overlay/apply.sh" "$control" "$cvt_overlay"
)

(
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
cd "$boot"
echo e66dc4f94208e50205a265e1a11dc299f4f7670dea25ef56eeb9dc8b405d8137  "%{SOURCE127}" | sha256sum -c --strict
git apply --check --whitespace=error %{SOURCE127}
git apply --whitespace=error %{SOURCE127}
)

(
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
notice=%{_builddir}/uke-control-boot-notify
echo 0d980ee45cbc919ea90a549df0da0fbfcf048786b47ba1830bd1b53545a5ba9a  "%{SOURCE129}" | sha256sum -c --strict
mkdir "$notice"
tar -xJf "%{SOURCE129}" -C "$notice"
(cd "$notice"; sha256sum -c --strict SHA256SUMS)
)
(
dock=%{_builddir}/uke-control-display-dock
echo d07cb0f74caf5de6fc8f136959789d3df967420134e9c80a6a244020fd356a36  "%{SOURCE130}" | sha256sum -c --strict
mkdir "$dock"
tar -xJf "%{SOURCE130}" -C "$dock"
(cd "$dock"; sha256sum -c --strict SHA256SUMS)
)
%build
(
    cd %{_builddir}/uke-cdsp-normal-firmware
    bash inputs/check.sh "$PWD/inputs" "$PWD/qualification.txt"
)
(
cd %{_builddir}/uke-control-display-dock
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags libsystemd) src/dock-session.cpp src/dock-desktop.cpp src/dock-layout.cpp %{build_ldflags} $(pkg-config --libs libsystemd) -o display-dock
)
(
cd %{_builddir}/uke-control-boot-notify
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags libsystemd) src/boot-notify.cpp %{build_ldflags} $(pkg-config --libs libsystemd) -o boot-notify
)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash %{SOURCE121} %{_builddir}/uke-control-inventory61/original.cpp "$control/src/backend.cpp" %{SOURCE120}
# UKE_PIDFD30_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-pidfd30
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$control/tests/execute-pidfd.sh" "$overlay/original.cpp" "$control/src/backend.cpp" "$control/tests"
)
# UKE_PIDFD30_BUILD_END
# UKE_UKECTL29_NATIVE_BEGIN
(
overlay=%{_builddir}/uke-control-ukectl29
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
for check in swap-ready bus-idle-wait; do
    if ! bash "$overlay/tests/$check.sh" "$control" > "$overlay/$check.txt" 2>&1; then
        tail -n 100 "$overlay/$check.txt"
        exit 1
    fi
    test "$(grep -c '^PASS: actual ' "$overlay/$check.txt")" = 4
    cat "$overlay/$check.txt"
done
)
# UKE_UKECTL29_NATIVE_END
# UKE_SWAP_READY28_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-swap-ready
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
if ! bash "$overlay/tests/swap-ready.sh" "$control" > "$overlay/results.txt" 2>&1;then tail -n 100 "$overlay/results.txt";exit 1;fi
cat "$overlay/results.txt"
test "$(grep -c '^PASS: actual swap startup' "$overlay/results.txt")" = 4
test "$(grep -c '^PASS: existing logical swap' "$overlay/results.txt")" = 4
)
# UKE_SWAP_READY28_BUILD_END
# UKE_FACTORY_INPUTS27_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-factory-inputs
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
color=%{_builddir}/uke-control-2.8.1/o82-build
if ! bash "$overlay/tests/factory-input.sh" "$boot" "$color" "$overlay" > "$overlay/results.txt" 2>&1;then tail -n 100 "$overlay/results.txt";exit 1;fi
cat "$overlay/results.txt"
test "$(grep -c '^PASS: actual factory input:' "$overlay/results.txt")" = 8
)
# UKE_FACTORY_INPUTS27_BUILD_END
# UKE_BOOT_STARTUP26_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-boot-startup
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
for check in boot-lock setup-cdc super-swap;do
 if ! bash "$control/tests/$check.sh" "$control" > "$overlay/$check-results.txt" 2>&1;then tail -n 100 "$overlay/$check-results.txt";exit 1;fi
 cat "$overlay/$check-results.txt"
 test "$(grep -c '^PASS: actual ' "$overlay/$check-results.txt")" = 4
done
)
# UKE_BOOT_STARTUP26_BUILD_END
# UKE_SUPER_SWAP25_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-super-swap
control=%{_builddir}/uke-control-boot-startup/legacy
for check in super-swap setup-cdc;do
 if ! bash "$control/tests/$check.sh" "$control" > "$overlay/$check-results.txt" 2>&1;then tail -n 100 "$overlay/$check-results.txt";exit 1;fi
 cat "$overlay/$check-results.txt"
 test "$(grep -c '^PASS: actual ' "$overlay/$check-results.txt")" = 4
done
)
# UKE_SUPER_SWAP25_BUILD_END
# UKE_SUPER_BOOT24_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-super-boot
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
# Keep the previous source seal before applying the incremental boot view.
(
cd "$boot"
sha256sum -c --strict --quiet "$overlay/post.sha256"
sha256sum -c --strict --quiet %{SOURCE100}
patch -p1 --batch --fuzz=0 --forward --dry-run -i %{SOURCE99}
patch -p1 --batch --fuzz=0 --forward -i %{SOURCE99}
sha256sum -c --strict --quiet %{SOURCE101}
)
current="$overlay/boot-image-view"
mkdir "$current"
# Carry forward all unchanged seals into the real ARM64 source test.
awk 'NR==FNR {changed[$2]=$1; next}
     !($2 in changed) {print}' %{SOURCE101} "$overlay/post.sha256" > "$current/post.sha256"
cat %{SOURCE101} >> "$current/post.sha256"
if ! bash "$boot/tests/super-boot-update.sh" "$boot" "$current" > "$overlay/native-results.txt" 2>&1;then tail -n 100 "$overlay/native-results.txt";exit 1;fi
cat "$overlay/native-results.txt"
test "$(grep -c '^PASS: actual super boot updater' "$overlay/native-results.txt")" = 4
)
# UKE_SUPER_BOOT24_BUILD_END
(
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
cd "$boot"
sha256sum -c --strict --quiet %{SOURCE104}
patch -p1 --batch --fuzz=0 --forward --dry-run -i %{SOURCE103}
patch -p1 --batch --fuzz=0 --forward -i %{SOURCE103}
sha256sum -c --strict --quiet %{SOURCE105}
printf '%%s  %%s\n' 35cf9b65b599c5680654d3569314dedd91dfa2dbe26b6fae1230a1e846bd647f %{SOURCE112} | sha256sum -c --strict --quiet -
bash %{SOURCE112} "$boot" "%{_builddir}/uke-control-2.8.1/o82-build"
git apply --check --whitespace=error %{SOURCE110}
git apply --whitespace=error %{SOURCE110}
git apply --reverse --check %{SOURCE110}
iris_prepare=$(mktemp -d)
sha256sum --strict -c <<'IRIS_SOURCE'
090bc8a7b1ac2dca436c128394f585446f276882a3d6af1dc1ca21f11c932b9d  %{SOURCE128}
IRIS_SOURCE
tar -xJf %{SOURCE128} -C "$iris_prepare"
(cd "$iris_prepare"; sha256sum -c --strict SHA256SUMS)
git apply --check --whitespace=error "$iris_prepare/prepare-iris-before-activation.patch"
git apply --whitespace=error "$iris_prepare/prepare-iris-before-activation.patch"
bash "$iris_prepare/check-prepare.sh" "$PWD/src/kernel-update.cpp"
git apply --reverse --check "$iris_prepare/prepare-iris-before-activation.patch"
notice=%{_builddir}/uke-control-boot-notify
sha256sum --strict -c "$notice/packaging/boot-before.sha256"
git apply --check --whitespace=error "$notice/packaging/boot-update.patch"
git apply --whitespace=error "$notice/packaging/boot-update.patch"
git apply --reverse --check "$notice/packaging/boot-update.patch"
install -m644 "$notice/shared/boot-notice.hpp" "$notice/packaging/boot-notice-publisher.hpp" src/
sha256sum --strict -c "$notice/packaging/boot-after.sha256"
sha256sum -c --strict <<'UKE_RUNNING_SLOT_SOURCES'
6897564cad82f2473106743ee4ac5cac8fda36de4883565d44f308c7747f8ede  %{SOURCE133}
91573f0c29fa43f2ac8d5deac395a370167fd7286c92fdf094e3a9ba8f22be8f  %{SOURCE134}
UKE_RUNNING_SLOT_SOURCES
git apply --check --whitespace=error %{SOURCE133}
git apply --whitespace=error %{SOURCE133}
git apply --reverse --check %{SOURCE133}
bash %{SOURCE134} src/kernel-update.cpp
)

# UKE_SETUP_CDC23_BUILD_BEGIN
(
control=%{_builddir}/uke-control-super-swap/legacy
overlay=%{_builddir}/uke-control-setup-cdc
(cd "$control";sha256sum -c --strict --quiet "$overlay/qualified-new-source.sha256")
if ! bash "$control/tests/setup-cdc.sh" "$control" > "$overlay/prepared-native-results.txt" 2>&1;then tail -n 80 "$overlay/prepared-native-results.txt";exit 1;fi
cat "$overlay/prepared-native-results.txt"
test "$(grep -c '^PASS: actual setup policy' "$overlay/prepared-native-results.txt")" = 4
)
# UKE_SETUP_CDC23_BUILD_END
(
%set_build_flags
cd %{_builddir}/uke-control-motion/uke-control
make -f packaging/motion.mk CXXFLAGS="$CXXFLAGS -std=c++20 -Wall -Wextra -Werror" LDFLAGS="$LDFLAGS" all
)
(
%set_build_flags
pogo_overlay=%{_builddir}/uke-control-pogo-integration
cd "$pogo_overlay/auth"
qcomtee=%{_builddir}/uke-qcomtee-fedora-sdk
PKG_CONFIG_PATH="$qcomtee/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" bash ../packaging/build-pogo-auth.sh
readelf -d build/uke-pogo-auth | grep -F 'Shared library: [libqcomtee.so.1]'
! readelf -d build/uke-pogo-auth | grep -Eq '\((RPATH|RUNPATH)\)'
cd "$pogo_overlay/desktop"
bash packaging/build.sh "$PWD/build"
)

(
cd %{_builddir}/uke-control-fastrpc-provider
%set_build_flags
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror src/ssc-cache.cpp -o uke-ssc-cache
)
(
cd %{_builddir}/uke-control-ssc-overlay/legacy
%set_build_flags
gcc $CFLAGS $LDFLAGS -std=gnu11 -Wall -Wextra -Werror -Wno-unused-parameter \
 -I include libhexagonrpc/*.c libhexagonrpc/interface/*.c hexagonrpcd/*.c \
 hexagonrpcd/interface/*.c -lsystemd -o uke-hexagonrpcd
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror ../inputs/ssc-cache.cpp -o uke-ssc-cache
)
(
cd %{_builddir}/uke-control-ssc-overlay/qualified
%set_build_flags
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror -fPIC -shared \
 -fno-exceptions -fno-rtti -fvisibility=hidden fastrpc-notify.cpp \
 $(pkg-config --cflags --libs libsystemd) -ldl -o libuke-fastrpc-notify.so
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \
 -I%{_builddir}/uke-fastrpc-fedora-reference/root%{_includedir} -Wl,--export-dynamic fastrpc-notify-test.cpp -ldl -o fixture

g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \
 -Ifiles/upstream -I%{_builddir}/uke-fastrpc-fedora-reference/root%{_includedir}/fastrpc files/fastrpc-files.cpp -ldl -o files-fixture

g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \
 session/fastrpc-session.cpp -ldl -o session-worker
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror -fPIC -shared \
 -I%{_builddir}/uke-fastrpc-fedora-reference/root%{_includedir} session/fastrpc-session-mocks.cpp -o session-mocks.so
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \
 session/fastrpc-session-test.cpp -o session-fixture
g++ $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror \
 -Isession/reference session/fastrpc-session-abi.cpp -o session-abi

)
(
cd %{_builddir}/uke-control-adaptive/runtime
%set_build_flags
cmake -S src/adreno-stats -B redhat-linux-build -G Ninja \
 -DCMAKE_C_FLAGS_RELEASE:STRING=-DNDEBUG -DCMAKE_CXX_FLAGS_RELEASE:STRING=-DNDEBUG \
 -DCMAKE_Fortran_FLAGS_RELEASE:STRING=-DNDEBUG -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
 -DCMAKE_INSTALL_DO_STRIP:BOOL=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr \
 -DCMAKE_INSTALL_FULL_SBINDIR:PATH=/usr/bin -DCMAKE_INSTALL_SBINDIR:PATH=bin \
 -DINCLUDE_INSTALL_DIR:PATH=/usr/include -DLIB_INSTALL_DIR:PATH=/usr/lib64 \
 -DSYSCONF_INSTALL_DIR:PATH=/etc -DSHARE_INSTALL_PREFIX:PATH=/usr/share \
 -DLIB_SUFFIX=64 -DBUILD_SHARED_LIBS:BOOL=ON
cmake --build redhat-linux-build -j"$RPM_BUILD_NCPUS" --verbose
bash %{_builddir}/uke-control-adaptive-overlay/check-adaptive-plugin-deps.sh \
 "$PWD/redhat-linux-build/libuke_adreno.so" %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json \
 %{_builddir}/uke-control-adaptive/plugin-elf.requires
)
battery_runtime=%{_builddir}/uke-control-battery/runtime
make -C "$battery_runtime" CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" "$battery_runtime/build/uke-battery" "$battery_runtime/build/uke-offline-charge"
for binary in uke-battery uke-offline-charge;do readelf -h "$battery_runtime/build/$binary" | grep -F AArch64;done
boot_charge=%{_builddir}/uke-control-boot-charge58
printf '%%s  %%s\n' 355e11cb2de0be4af16711236c1ab3c4c9a9b063014c415a1a7fcf9db6ad6f76 %{SOURCE118} | sha256sum -c --strict --quiet -
mkdir "$boot_charge"
tar -xJf %{SOURCE118} -C "$boot_charge"
(cd "$boot_charge"; sha256sum -c --strict --quiet SHA256SUMS)
(
cd %{_builddir}/uke-control-charge/runtime
sha256sum -c --strict --quiet "$boot_charge/charge-before.sha256"
git apply --check --whitespace=error "$boot_charge/charge-after-wait.patch"
git apply --whitespace=error "$boot_charge/charge-after-wait.patch"
sha256sum -c --strict --quiet "$boot_charge/charge-after.sha256"
%{__cxx} %{build_cxxflags} -std=c++17 -UNDEBUG -Wall -Wextra -Werror tests/charge-control.cpp src/battery/control.cpp %{build_ldflags} -o "$boot_charge/test-charge-control"
"$boot_charge/test-charge-control"
)
bash "$boot_charge/vrr/tests/vrr-load.sh"
make -C %{_builddir}/uke-control-charge/runtime CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" %{_builddir}/uke-control-charge/runtime/build/uke-charge-control
readelf -h %{_builddir}/uke-control-charge/runtime/build/uke-charge-control | grep -F AArch64
%{__cxx} %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror -I%{_builddir}/uke-control-nfc/nfc/include -I%{_builddir}/uke-control-nfc/nfc/src %{_builddir}/uke-control-nfc/nfc/src/main.cpp %{build_ldflags} -o %{_builddir}/uke-control-nfc/uke-nfc-tag
bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-ensure-loaded.sh %{_builddir}/uke-control-speaker
bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-provider.sh %{_builddir}/uke-control-speaker/src/audio/speaker-gain-preflight.sh
bash %{_builddir}/uke-control-brightness/tests/brightness-curve.sh
bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/critical-health.sh
bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/boot-health-report.sh
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"

  CFLAGS="${CFLAGS:--O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer  }" ; export CFLAGS ; 
  CXXFLAGS="${CXXFLAGS:--O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer  }" ; export CXXFLAGS ; 
  FFLAGS="${FFLAGS:--O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -I/usr/lib64/gfortran/modules  }" ; export FFLAGS ; 
  FCFLAGS="${FCFLAGS:--O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -I/usr/lib64/gfortran/modules  }" ; export FCFLAGS ; 
  VALAFLAGS="${VALAFLAGS:--g}" ; export VALAFLAGS ;
  RUSTFLAGS="${RUSTFLAGS:--Copt-level=3 -Cdebuginfo=2 -Ccodegen-units=1 -Cstrip=none -Cforce-frame-pointers=yes -Clink-arg=-specs=/usr/lib/rpm/redhat/redhat-package-notes --cap-lints=warn}" ; export RUSTFLAGS ; 
  LDFLAGS="${LDFLAGS:--Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  }" ; export LDFLAGS ; 
  LT_SYS_LIBRARY_PATH="${LT_SYS_LIBRARY_PATH:-/usr/lib64:}" ; export LT_SYS_LIBRARY_PATH ; 
  CC="${CC:-gcc}" ; export CC ; 
  CXX="${CXX:-g++}" ; export CXX
/usr/bin/make -O -j${RPM_BUILD_NCPUS} V=1 VERBOSE=1
# UKE_UKECTL29_BUILD_BEGIN
read -r -a capacity_flags <<< "$(pkg-config --cflags --libs ext2fs mount)"
"$CXX" $CXXFLAGS $LDFLAGS -std=c++20 -Wall -Wextra -Werror -Isrc \
    -o build/ukectl src/ukectl.cpp -Lbuild -l:libuke-control.so.0 "${capacity_flags[@]}"
CXX="$CXX" bash tests/root-capacity.sh
# UKE_UKECTL29_BUILD_END


)


)

contract=%{_builddir}/uke-control-frontend-contract
mapfile -t client_libraries < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/build/libuke-control-client.so.1')
test ${#client_libraries[@]} = 1
library=${client_libraries[0]}
cmp "$contract/client.hpp" "${library%/build/libuke-control-client.so.1}/src/client.hpp"
g++ %{build_cxxflags} -std=c++20 -Wall -Wextra -I"$contract" "$contract/frontend-request.cpp" "$library" -o "$contract/frontend-request"
printf '%s\n' "${library%/*}" > "$contract/library-directory"

(
cd "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102"
ulimit -c 0
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I src "-DUKE_TOUCH_HEALTH_SOURCE=\"$PWD/tests/fixtures/touch-health-before-stats.hpp\"" tests/touch-health.cpp -o touch-health-before
set +e
./touch-health-before > touch-health-before.log 2>&1
touch_health_status=$?
set -e
if test "$touch_health_status" != 134; then cat touch-health-before.log; exit 1; fi
grep -Fq 'Additive touch statistics must retain readiness admission' touch-health-before.log
echo 'Original touch-health consumer rejects additive statistics (expected status134)'
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -ffile-prefix-map=$PWD=. src/boot-status.cpp -o uke-boot-status -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/usb-port.cpp -o uke-usb-port $(pkg-config --cflags --libs libsystemd libdrm) -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -ffile-prefix-map=$PWD=. src/esp32-cdc.cpp -o uke-esp32-cdc -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/wifi-factory.cpp -o uke-wifi-factory -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/bluetooth-address.cpp -o uke-bluetooth-address -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/boot-guard.cpp -o uke-boot-guard $(pkg-config --cflags --libs libsystemd wayland-client) -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/reboot.cpp -o uke-reboot $(pkg-config --cflags --libs libsystemd) -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/kernel-update.cpp -o uke-kernel-update $(pkg-config --cflags --libs fdisk zlib openssl libsystemd) -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/install-profile.cpp -o uke-install-profile -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/native-profile-ready.cpp -o uke-native-power-profile-ready $(pkg-config --cflags --libs libsystemd) -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/touch-launcher.cpp -o uke-touchscreen -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  
g++ -O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -mbranch-protection=standard -fasynchronous-unwind-tables -fstack-clash-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -std=c++20 -Wall -Wextra -Werror -Wno-unused-function -ffile-prefix-map=$PWD=. touch-runtime/legacy/src/touch-thp.cpp touch-runtime/legacy/src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -lsystemd -o uke-touch-thp -Wl,-z,relro -Wl,--as-needed  -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1  -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes  

)


(
mkdir -p '%{_builddir}/uke-control-2.8.1/o82-build/'
cd '%{_builddir}/uke-control-2.8.1/o82-build/'
g++ -std=c++20 %{optflags} -UNDEBUG -Wall -Wextra -Werror \
    o82-color-profile.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) \
    %{build_ldflags} -o uke-o82-color-profile

)


(
cd %{_builddir}/uke-control-session
%{__cxx} %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror $(pkg-config --cflags alsa wayland-client libsystemd) src/session/speaker-orientation.cpp %{build_ldflags} $(pkg-config --libs alsa wayland-client libsystemd) -o uke-speaker-orientation
)
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags jsoncpp libsystemd) %{_builddir}/uke-control-brightness/src/brightness-curve.cpp %{build_ldflags} $(pkg-config --libs jsoncpp libsystemd) -o %{_builddir}/uke-control-brightness/brightness-curve
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror %{_builddir}/uke-control-speaker/src/audio/uke-speaker-runtime.cpp %{build_ldflags} -lasound -lcrypto -o %{_builddir}/uke-control-speaker/uke-speaker-runtime
%install
o82_effect=%{buildroot}%{_datadir}/kwin/effects/uke-o82-resolution-continuity
install -Dm644 %{SOURCE135} "$o82_effect/metadata.json"
install -Dm644 %{SOURCE136} "$o82_effect/contents/code/main.js"
cmp %{SOURCE135} "$o82_effect/metadata.json"
cmp %{SOURCE136} "$o82_effect/contents/code/main.js"
(
dock=%{_builddir}/uke-control-display-dock
install -Dm755 "$dock/display-dock" %{buildroot}%{_libexecdir}/uke-control/display-dock
install -Dm644 "$dock/packaging/uke-control-display-dock.service" %{buildroot}%{_userunitdir}/uke-control-display-dock.service
install -d %{buildroot}%{_userunitdir}/graphical-session.target.wants
ln -s ../uke-control-display-dock.service %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-control-display-dock.service
)
(
notice=%{_builddir}/uke-control-boot-notify
install -Dm755 "$notice/boot-notify" %{buildroot}%{_libexecdir}/uke-control/boot-notify
install -Dm644 "$notice/packaging/uke-boot-notify.service" %{buildroot}%{_userunitdir}/uke-boot-notify.service
install -Dm644 "$notice/packaging/uke-boot-notify.path" %{buildroot}%{_userunitdir}/uke-boot-notify.path
install -d %{buildroot}%{_userunitdir}/graphical-session.target.wants
ln -s ../uke-boot-notify.service %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.service
ln -s ../uke-boot-notify.path %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.path
install -Dm644 "$notice/packaging/50-uke-boot-notify.conf" %{buildroot}%{_unitdir}/uke-userdata-fallback.service.d/50-uke-boot-notify.conf
install -Dm644 "$notice/packaging/org.senemos.uke.kernel-notification.policy" %{buildroot}%{_datadir}/polkit-1/actions/org.senemos.uke.kernel-notification.policy
install -dm755 %{buildroot}%{_sharedstatedir}/senemos/kernel-notifications
)
motion=%{_builddir}/uke-control-motion/uke-control
install -Dm755 "$motion/build/motion/motion" %{buildroot}/usr/libexec/uke-control/motion
install -Dm755 "$motion/packaging/selinux/motion-policy.sh" %{buildroot}/usr/libexec/uke-control/motion-policy
install -Dm644 "$motion/build/motion/selinux/uke_control_motion.cil" %{buildroot}/usr/share/selinux/packages/uke_control_motion.cil
install -Dm644 "$motion/packaging/uke-control-motion.service" %{buildroot}/usr/lib/systemd/system/uke-control-motion.service
install -Dm644 "$motion/packaging/org.senemos.Uke.Motion1.service" %{buildroot}/usr/share/dbus-1/system-services/org.senemos.Uke.Motion1.service
install -Dm644 "$motion/packaging/org.senemos.Uke.Motion1.conf" %{buildroot}/usr/share/dbus-1/system.d/org.senemos.Uke.Motion1.conf
install -Dm644 "$motion/packaging/org.senemos.uke.motion.policy" %{buildroot}/usr/share/polkit-1/actions/org.senemos.uke.motion.policy
install -Dm644 "$motion/docs/MOTION1.md" %{buildroot}%{_docdir}/uke-control/MOTION1.md
pogo_overlay=%{_builddir}/uke-control-pogo-integration
install -Dm755 "$pogo_overlay/auth/build/uke-pogo-auth" %{buildroot}/usr/libexec/uke-pogo-auth
install -Dm755 "$pogo_overlay/desktop/build/uke-pogo-desktop" %{buildroot}/usr/libexec/uke-pogo-desktop
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/packaging/uke-pogo-auth.service" %{buildroot}/usr/lib/systemd/system/uke-pogo-auth.service
install -Dm644 "$pogo_overlay/desktop/packaging/pogo-attachment.conf" %{buildroot}/usr/lib/systemd/system/uke-pogo-auth.service.d/pogo-attachment.conf
install -Dm644 "$pogo_overlay/desktop/packaging/uke-pogo-desktop.service" %{buildroot}/usr/lib/systemd/user/uke-pogo-desktop.service
install -Dm644 "$pogo_overlay/desktop/packaging/70-uke-pogo.rules" %{buildroot}/usr/lib/udev/rules.d/70-uke-pogo.rules
install -Dm644 "$pogo_overlay/desktop/packaging/uke-pogo-desktop.desktop" %{buildroot}/etc/xdg/autostart/uke-pogo-desktop.desktop
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/MIT-LICENSE.txt
for library in minkipc quic-teec;do
 install -Dm644 "$pogo_overlay/auth/upstream/$library/LICENSE.txt" "%{buildroot}%{_licensedir}/%{name}/pogo/$library-LICENSE.txt"
done
install -Dm644 "$pogo_overlay/auth/upstream/qcbor/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/qcbor-LICENSE.txt
install -Dm644 "$pogo_overlay/auth/upstream/quic-teec/libqcomtee/src/linux/tee.h" %{buildroot}%{_licensedir}/%{name}/pogo/LINARO-tee-interface-notice.h
install -Dm644 "$pogo_overlay/desktop/LICENSE" %{buildroot}%{_licensedir}/%{name}/pogo/DESKTOP-LICENSE.txt
install -Dm644 "$pogo_overlay/auth/uke-pogo-auth/README.md" %{buildroot}%{_docdir}/%{name}/POGO-AUTH.md
install -Dm644 "$pogo_overlay/desktop/README.md" %{buildroot}%{_docdir}/%{name}/POGO-DESKTOP.md
install -Dm644 "$pogo_overlay/docs/POGO-INTEGRATION.md" %{buildroot}%{_docdir}/%{name}/POGO-INTEGRATION.md

fastrpc_overlay=%{_builddir}/uke-control-fastrpc-provider
for unit_file in "$fastrpc_overlay"/src/fastrpc/*;do
 install -Dm644 "$unit_file" "%{buildroot}/usr/lib/systemd/system/$(basename "$unit_file")"
done
bash "$fastrpc_overlay/packaging/stage-fastrpc-root.sh" %{buildroot}/usr/share/uke-control/fastrpc-root
install -Dm644 "$fastrpc_overlay/docs/FASTRPC-PROVIDER.md" %{buildroot}%{_docdir}/%{name}/FASTRPC-PROVIDER.md
ssc=%{_builddir}/uke-control-ssc-overlay
# UKE_FASTRPC_RETIRED_BLOCK_4
install -Dm755 "$fastrpc_overlay/uke-ssc-cache" %{buildroot}/usr/libexec/uke-ssc-cache
# UKE_FASTRPC_RETIRED_BLOCK_6
install -Dm644 "$ssc/inputs/iio-ssc-order.conf" %{buildroot}/usr/lib/systemd/system/iio-sensor-proxy.service.d/uke-ssc.conf
install -Dm644 "$ssc/inputs/70-uke-ssc.rules" %{buildroot}/usr/lib/udev/rules.d/70-uke-ssc.rules
install -Dm644 "$ssc/legacy/COPYING" %{buildroot}/usr/share/licenses/uke-ssc-fileservice/COPYING
install -Dm755 "$ssc/qualified/session-worker" %{buildroot}/usr/libexec/uke-control/fastrpc-session
install -Dm755 "$ssc/qualified/libuke-fastrpc-notify.so" %{buildroot}/usr/lib64/uke-control/libuke-fastrpc-notify.so
adaptive=%{_builddir}/uke-control-adaptive/runtime
DESTDIR=%{buildroot} cmake --install "$adaptive/redhat-linux-build"
for item in adreno.page;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/usr/share/plasma-systemmonitor/$item;done
install -Dm644 "$adaptive/packaging/uke-adaptive-display.target" %{buildroot}/usr/lib/systemd/system/uke-adaptive-display.target
install -Dm644 "$adaptive/packaging/80-uke-adaptive-display.preset" %{buildroot}/usr/lib/systemd/system-preset/80-uke-adaptive-display.preset
for item in 71-uke-tablet-mode.rules 61-uke-display-ddc.rules;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/usr/lib/udev/rules.d/$item;done
install -Dm644 "$adaptive/packaging/60-block-scheduler.rules" %{buildroot}/etc/udev/rules.d/60-block-scheduler.rules
for item in powerdevilrc kwinoutputconfig.json;do install -Dm644 "$adaptive/packaging/$item" %{buildroot}/etc/xdg/$item;done
install -Dm755 "$adaptive/src/uke-ddc-config" %{buildroot}/usr/libexec/senemos-uke/uke-ddc-config
install -Dm644 "$adaptive/packaging/60-uke-display-ddc.conf" %{buildroot}/usr/lib/systemd/user/plasma-powerdevil.service.d/60-uke-display-ddc.conf
install -Dm644 "$adaptive/packaging/30-uke-s2idle.conf" %{buildroot}/usr/lib/systemd/sleep.conf.d/30-uke-s2idle.conf
install -Dm644 "$adaptive/packaging/uke-suspend.conf" %{buildroot}/usr/lib/tmpfiles.d/uke-suspend.conf
for item in ADAPTIVE-DISPLAY.md DISPLAY-CONTROLS.md SUSPEND.md SERVICE-POLICY.md GPU-MONITORING.md;do
 install -Dm644 "$adaptive/docs/$item" %{buildroot}/usr/share/doc/uke-adaptive-display/$item
done
install -Dm644 "$adaptive/LICENSE" %{buildroot}/usr/share/licenses/uke-adaptive-display/LICENSE
battery_runtime=%{_builddir}/uke-control-battery/runtime
install -Dm755 "$battery_runtime/build/uke-battery" %{buildroot}%{_bindir}/uke-battery
install -Dm755 "$battery_runtime/build/uke-offline-charge" %{buildroot}%{_libexecdir}/uke-offline-charge
for unit in uke-battery.service uke-offline-charge.service uke-offline-charge.target;do
 install -Dm644 "$battery_runtime/packaging/systemd/$unit" %{buildroot}%{_unitdir}/$unit
done
install -Dm644 "$battery_runtime/packaging/systemd/90-uke-battery.preset" %{buildroot}%{_presetdir}/90-uke-battery.preset
install -Dm644 %{SOURCE54} %{buildroot}%{_sysusersdir}/uke-battery.conf
for item in org.senemos.Uke.Battery1.conf org.senemos.Uke.Battery1.service;do
 case $item in *.conf) directory=system.d;; *.service) directory=system-services;; esac
 install -Dm644 "$battery_runtime/packaging/dbus/$item" %{buildroot}%{_datadir}/dbus-1/$directory/$item
done
for item in battery-profile.json model-profile.json;do install -Dm644 "$battery_runtime/manifests/$item" %{buildroot}%{_datadir}/uke-battery/$item;done
for item in README.md SOURCE-SHA256SUMS pack-variant-source.json;do install -Dm644 "$battery_runtime/$item" %{buildroot}%{_docdir}/uke-battery-observer/$item;done
for item in BATTERY.md RUNTIME-PROFILE.md BATTERY-ENGINEERING.md;do install -Dm644 "$battery_runtime/docs/$item" %{buildroot}%{_docdir}/uke-battery-observer/$item;done
install -Dm644 "$battery_runtime/docs/OFFLINE-CHARGING.md" %{buildroot}%{_docdir}/uke-offline-charge/OFFLINE-CHARGING.md
for package in uke-battery-observer uke-offline-charge;do install -Dm644 "$battery_runtime/LICENSE" %{buildroot}%{_licensedir}/$package/LICENSE;done
charge_runtime=%{_builddir}/uke-control-charge/runtime
install -Dm755 "$charge_runtime/build/uke-charge-control" %{buildroot}%{_libexecdir}/uke-charge-control
install -Dm644 "$charge_runtime/manifests/protection-readiness.json" %{buildroot}%{_datadir}/uke-battery/protection-readiness.json
for unit in control automatic protection;do
 install -Dm644 "$charge_runtime/packaging/systemd/uke-charge-$unit.service" %{buildroot}%{_unitdir}/uke-charge-$unit.service
done
printf '%s  %s\n' d64665c8183d12617def9e085802344eeeacc9dcd76d9b8f10da9cfb9d2cb381 %{SOURCE106} | sha256sum -c --strict --quiet -
udevadm verify --no-summary %{SOURCE106}
install -Dm644 %{SOURCE106} %{buildroot}%{_udevrulesdir}/80-uke-charge-protection.rules
printf '%s  %s\n' 0d17a6b81e6556b334e7c91c39e4e338d1d3dd437de8a26355ac9cc98ccf6ff2 %{SOURCE107} | sha256sum -c --strict --quiet -
install -Dm644 %{SOURCE107} %{buildroot}%{_unitdir}/uke-charge-protection.service
cmp %{SOURCE107} %{buildroot}%{_unitdir}/uke-charge-protection.service
install -Dm644 "$charge_runtime/packaging/systemd/91-uke-charge-control.preset" %{buildroot}%{_presetdir}/91-uke-charge-control.preset
install -Dm644 "$charge_runtime/manifests/control-profile.json" %{buildroot}%{_sysconfdir}/uke-battery/control-profile.json
install -Dm644 "$charge_runtime/packaging/upower/60-uke-battery-safety.conf" %{buildroot}%{_sysconfdir}/UPower/UPower.conf.d/60-uke-battery-safety.conf
for doc in CHARGE-CONTROL.md CHARGING-PROTOCOLS.md BATTERY-LONGEVITY.md CHARGE-PUMP-PROTECTION.md OSS-CHARGING-INVENTORY.tsv CHARGING-DOCUMENTS.tsv;do
 install -Dm644 "$charge_runtime/docs/$doc" %{buildroot}%{_docdir}/uke-charge-control/$doc
done
install -Dm644 "$charge_runtime/LICENSE" %{buildroot}%{_licensedir}/uke-charge-control/LICENSE
install -Dm644 %{SOURCE42} %{buildroot}/usr/lib/udev/rules.d/89-uke-native-proximity.rules
install -Dm644 %{SOURCE43} %{buildroot}%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md
install -Dm755 %{_builddir}/uke-control-nfc/uke-nfc-tag %{buildroot}/usr/bin/uke-nfc-tag
install -Dm644 %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
install -Dm644 %{_builddir}/uke-control-nfc/nfc/README.md %{buildroot}%{_docdir}/uke-control/NFC-IR.md
install -Dm644 %{_builddir}/uke-control-nfc/nfc/uapi-source.json %{buildroot}%{_docdir}/uke-control/nfc-uapi-source.json
install -Dm644 %{_builddir}/uke-control-nfc/NFC-CONSOLIDATION.md %{buildroot}%{_docdir}/uke-control/NFC-CONSOLIDATION.md
install -Dm644 %{_builddir}/uke-control-nfc/NFC-SOURCE.json %{buildroot}%{_docdir}/uke-control/NFC-SOURCE.json
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control
ln -s uke-control %{buildroot}/usr/bin/ukectl

install -Dm644 packaging/50-uke-display-handoff.conf \
    %{buildroot}%{_unitdir}/plymouth-quit.service.d/50-uke-display-handoff.conf
install -Dm644 packaging/50-uke-display-handoff-wait.conf \
    %{buildroot}%{_unitdir}/plymouth-quit-wait.service.d/50-uke-display-handoff.conf
# UKE_SWAP_READY28_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-swap-ready
install -Dm644 "$overlay/uke-swap-ready.service" %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service
install -Dm644 "$overlay/40-uke-swap-ready.conf" %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
install -Dm644 "$overlay/40-uke-swap-startup.conf" %{buildroot}/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
install -Dm644 "$overlay/40-uke-swap-before-session.conf" %{buildroot}/usr/lib/systemd/system/systemd-user-sessions.service.d/40-uke-swap-before-session.conf
install -Dm644 "$overlay/results.txt" %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt
install -Dm644 "$overlay/docs/SWAP-STARTUP.md" %{buildroot}%{_docdir}/uke-control/swap-ready28/SWAP-STARTUP.md
# UKE_SWAP_READY28_INSTALL_END
# UKE_FACTORY_INPUTS27_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-factory-inputs
install -Dm644 "$overlay/results.txt" %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt
install -Dm644 "$overlay/docs/FACTORY-UPDATE-INPUTS.md" %{buildroot}%{_docdir}/uke-control/factory-inputs27/FACTORY-UPDATE-INPUTS.md
# UKE_FACTORY_INPUTS27_INSTALL_END
# UKE_BOOT_STARTUP26_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-boot-startup
for name in boot-lock setup-cdc super-swap;do
 install -Dm644 "$overlay/$name-results.txt" %{buildroot}%{_docdir}/uke-control/startup26/$name-results.txt
done
# UKE_BOOT_STARTUP26_INSTALL_END
# UKE_SUPER_SWAP25_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-super-swap
install -Dm755 "$overlay/prepare.sh" %{buildroot}/usr/libexec/uke-control/super-swap/prepare.sh
for file in "$overlay/runtime/"*.sh;do install -m755 "$file" %{buildroot}/usr/libexec/uke-control/super-swap/;done
install -Dm644 "$overlay/docs/LOGICAL-SWAP.md" %{buildroot}%{_docdir}/uke-control/LOGICAL-SWAP.md
install -m644 "$overlay/super-swap-results.txt" "$overlay/setup-cdc-results.txt" %{buildroot}%{_docdir}/uke-control/
# UKE_SUPER_SWAP25_INSTALL_END
# UKE_SUPER_BOOT24_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-super-boot
install -Dm644 "$overlay/docs/SUPER-ROOT-BOOT.md" %{buildroot}%{_docdir}/uke-control/SUPER-ROOT-BOOT.md
install -m644 "$overlay/native-results.txt" %{buildroot}%{_docdir}/uke-control/super-boot-native-results.txt
# UKE_SUPER_BOOT24_INSTALL_END
# UKE_SETUP_CDC23_INSTALL_BEGIN
setup=%{_builddir}/uke-control-setup-cdc/packaging
for unit in uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path;do
 install -Dm644 "$setup/$unit" "%{buildroot}/usr/lib/systemd/system/$unit"
done
install -Dm644 "$setup/91-uke-control-setup-cdc.rules" %{buildroot}/usr/lib/udev/rules.d/91-uke-control-setup-cdc.rules
install -Dm644 "$setup/81-uke-control-setup-cdc.preset" %{buildroot}/usr/lib/systemd/system-preset/81-uke-control-setup-cdc.preset
# Activation is reserved for an explicitly marked fresh Plasma image.
test ! -e %{buildroot}/etc/uke-control/first-setup-cdc
# UKE_SETUP_CDC23_INSTALL_END
install -Dm644 src/uke-microphone.lua %{buildroot}/usr/share/wireplumber/scripts/uke-microphone.lua
install -Dm644 src/uke-microphone-policy.lua %{buildroot}/usr/share/wireplumber/scripts/lib/uke-microphone-policy.lua
install -Dm644 packaging/96-uke-microphone-policy.conf %{buildroot}/usr/share/wireplumber/wireplumber.conf.d/96-uke-microphone-policy.conf
install -Dm755 build/uke-control-tui %{buildroot}/usr/bin/uke-control-tui
install -Dm755 build/libuke-control-client.so.1 %{buildroot}/usr/lib64/libuke-control-client.so.1
ln -s libuke-control-client.so.1 %{buildroot}/usr/lib64/libuke-control-client.so
install -Dm644 src/client.hpp %{buildroot}/usr/include/uke-control/client.hpp
install -Dm644 build/uke-control-client.pc %{buildroot}/usr/lib64/pkgconfig/uke-control-client.pc
install -Dm644 packaging/uke-control.bash %{buildroot}/usr/share/bash-completion/completions/uke-control
install -Dm755 build/uke-control-rgb %{buildroot}/usr/libexec/uke-control/rgb-led
install -Dm755 build/torch-access %{buildroot}/usr/libexec/uke-control/torch-access
install -Dm644 packaging/91-uke-control-torch.rules %{buildroot}/usr/lib/udev/rules.d/91-uke-control-torch.rules
install -Dm644 packaging/org.senemos.Uke.Torch.conf %{buildroot}/usr/share/dbus-1/system.d/org.senemos.Uke.Torch.conf
install -Dm644 packaging/org.senemos.Uke.Torch.service %{buildroot}/usr/share/dbus-1/system-services/org.senemos.Uke.Torch.service
install -Dm644 packaging/org.senemos.uke.torch.policy %{buildroot}/usr/share/polkit-1/actions/org.senemos.uke.torch.policy
install -Dm644 build/selinux/uke_control_rgb.pp %{buildroot}/usr/share/selinux/packages/uke_control_rgb.pp
install -Dm644 packaging/uke-control-rgb.service %{buildroot}/usr/lib/systemd/system/uke-control-rgb.service
install -Dm644 packaging/uke-control-rgb.sysusers %{buildroot}/usr/lib/sysusers.d/uke-control-rgb.conf
install -Dm644 packaging/91-uke-control-rgb.rules %{buildroot}/usr/lib/udev/rules.d/91-uke-control-rgb.rules
install -Dm644 packaging/org.senemos.Uke.Rgb.conf %{buildroot}/usr/share/dbus-1/system.d/org.senemos.Uke.Rgb.conf
install -Dm644 packaging/org.senemos.Uke.Rgb.service %{buildroot}/usr/share/dbus-1/system-services/org.senemos.Uke.Rgb.service
install -Dm644 packaging/org.senemos.uke.rgb.policy %{buildroot}/usr/share/polkit-1/actions/org.senemos.uke.rgb.policy
install -Dm755 build/uke-control-packages %{buildroot}/usr/libexec/uke-control/packages
install -Dm755 build/resize-system %{buildroot}/usr/libexec/uke-control/resize-system
install -Dm755 build/layout-manager %{buildroot}/usr/libexec/uke-control/layout-manager
install -Dm644 packaging/91-uke-control-layout.rules %{buildroot}/usr/lib/udev/rules.d/91-uke-control-layout.rules
sha256sum -c --strict --quiet %{_builddir}/uke-control-boot-charge58/vrr-source.sha256
install -Dm755 %{_builddir}/uke-control-boot-charge58/vrr/packaging/vrr-min-load %{buildroot}/usr/libexec/uke-control/vrr-min-load
policy_source=%{_builddir}/uke-control-suspend-dsc-policy
mkdir -p "$policy_source"
tar -xJf %{SOURCE138} -C "$policy_source"
install -Dm755 "$policy_source/packaging/hardware-policy" %{buildroot}/usr/libexec/uke-control/hardware-policy
install -Dm755 "$policy_source/packaging/vrr-min-load" %{buildroot}/usr/libexec/uke-control/vrr-min-load
install -Dm644 "$policy_source/packaging/uke-control-cpu-sleep.service" %{buildroot}/usr/lib/systemd/system/uke-control-cpu-sleep.service
install -Dm644 "$policy_source/packaging/90-uke-cpu-sleep.conf" %{buildroot}/usr/lib/systemd/system/systemd-suspend.service.d/90-uke-cpu-sleep.conf
install -Dm644 "$policy_source/packaging/org.senemos.uke.hardware-policy.policy" %{buildroot}/usr/share/polkit-1/actions/org.senemos.uke.hardware-policy.policy
install -Dm644 packaging/60-uke-control-vrr.conf %{buildroot}/usr/lib/modprobe.d/60-uke-control-vrr.conf
install -Dm755 build/libuke-control.so.0 %{buildroot}/usr/lib64/libuke-control.so.0
install -Dm644 src/backend.hpp %{buildroot}/usr/include/uke-control/backend.hpp
install -Dm644 src/charge-policy.hpp %{buildroot}/usr/include/uke-control/charge-policy.hpp
install -Dm644 src/front-color.hpp %{buildroot}/usr/include/uke-control/front-color.hpp
install -Dm644 src/desktop.hpp %{buildroot}/usr/include/uke-control/desktop.hpp
install -Dm644 src/vrr.hpp %{buildroot}/usr/include/uke-control/vrr.hpp
install -Dm755 build/uke-control-tests %{buildroot}/usr/libexec/uke-control/tests
install -Dm755 build/ssc-discovery %{buildroot}/usr/libexec/uke-control/ssc-discovery
install -Dm755 build/ssc-sample %{buildroot}/usr/libexec/uke-control/ssc-sample
install -Dm644 packaging/uke-control.service %{buildroot}/usr/lib/systemd/system/uke-control.service
# UKE_UKECTL29_INSTALL_BEGIN
install -Dm755 build/ukectl %{buildroot}/usr/libexec/uke-control/ukectl
ln -s uke-control.service %{buildroot}/usr/lib/systemd/system/ukectl.service
# UKE_UKECTL29_INSTALL_END
install -Dm644 packaging/uke-control-wake.service %{buildroot}/usr/lib/systemd/system/uke-control-wake.service
install -Dm644 packaging/uke-control-cdc.service %{buildroot}/usr/lib/systemd/system/uke-control-cdc.service
install -Dm644 packaging/uke-control-flashlight.service %{buildroot}/usr/lib/systemd/system/uke-control-flashlight.service
install -Dm644 packaging/90-uke-control-cdc.rules %{buildroot}/usr/lib/udev/rules.d/90-uke-control-cdc.rules
install -Dm644 %{SOURCE137} %{buildroot}%{_unitdir}/serial-getty@ttyGS0.service.d/40-uke-serial-reset.conf
install -d %{buildroot}/usr/lib/systemd/system/sleep.target.requires
ln -s ../uke-control-wake.service %{buildroot}/usr/lib/systemd/system/sleep.target.requires/uke-control-wake.service
install -Dm644 packaging/80-uke-control.preset %{buildroot}/usr/lib/systemd/system-preset/80-uke-control.preset
install -Dm644 packaging/uke-control-moisture-notify.service %{buildroot}/usr/lib/systemd/user/uke-control-moisture-notify.service
install -Dm644 packaging/80-uke-control-user.preset %{buildroot}/usr/lib/systemd/user-preset/80-uke-control-user.preset
install -Dm644 packaging/uke-control-tests.desktop %{buildroot}/usr/share/applications/uke-control-tests.desktop
install -d -m700 %{buildroot}/var/lib/uke-control


)


)

contract=%{_builddir}/uke-control-frontend-contract
install -Dm644 "$contract/frontend.hpp" %{buildroot}%{_includedir}/uke-control/frontend.hpp
install -Dm755 "$contract/frontend-request" %{buildroot}%{_libexecdir}/uke-control/frontend-request
package_root=$(dirname "$(cat "$contract/library-directory")")
install -Dm755 "$package_root/build/update-status" %{buildroot}%{_libexecdir}/uke-control/update-status
install -Dm755 "$package_root/build/set-slot" %{buildroot}%{_libexecdir}/uke-control/set-slot
install -Dm644 "$package_root/packaging/org.senemos.uke.update-status.policy" %{buildroot}%{_datadir}/polkit-1/actions/org.senemos.uke.update-status.policy

(
cd "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102"
install -Dm644 src/profiles/60-uke-security.conf %{buildroot}/usr/lib/sysctl.d/60-uke-security.conf
install -Dm755 uke-touchscreen %{buildroot}/usr/libexec/senemos-uke/uke-touchscreen
install -Dm755 uke-touch-thp %{buildroot}/usr/libexec/senemos-uke/uke-touch-thp
install -d %{buildroot}/usr/lib/systemd/system
sed 's@ExecStart=/usr/libexec/senemos-uke/uke-touch-thp --daemon@ExecCondition=/usr/libexec/senemos-uke/uke-touchscreen --userspace-needed\nExecStart=/usr/libexec/senemos-uke/uke-touchscreen --daemon@' touch-runtime/legacy/src/units/uke-touchscreen.service > %{buildroot}/usr/lib/systemd/system/uke-touchscreen.service
install -Dm644 touch-runtime/legacy/src/units/80-uke-touchscreen.preset %{buildroot}/usr/lib/systemd/system-preset/80-uke-touchscreen.preset
install -Dm644 touch-runtime/legacy/src/udev/70-uke-touchscreen.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-touchscreen.rules
install -Dm644 touch-runtime/legacy/src/modules-load.d/uke-touchscreen.conf %{buildroot}/usr/lib/modules-load.d/uke-touchscreen.conf
install -Dm644 touch-runtime/legacy/src/modprobe.d/uke-touchscreen.conf %{buildroot}/usr/lib/modprobe.d/uke-touchscreen.conf
install -d %{buildroot}/usr/lib/firmware/novatek/uke
ln -s ../../qcom/uke/touch/csot/normal.bin %{buildroot}/usr/lib/firmware/novatek/uke/nt36532-normal.bin
install -Dm644 src/units/70-uke-chronyd-clock.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/70-uke-clock.conf
install -Dm644 src/profiles/uke-system-heap.conf %{buildroot}/usr/lib/modules-load.d/uke-system-heap.conf
install -Dm644 src/udev/72-uke-system-heap.rules %{buildroot}/usr/lib/udev/rules.d/72-uke-system-heap.rules
install -pDm644 src/profiles/clock-epoch %{buildroot}/usr/lib/clock-epoch
install -Dm644 src/units/30-uke-plasmalogin-ready.conf %{buildroot}%{_unitdir}/plasmalogin.service.d/30-uke-native-drm.conf
install -Dm755 src/shutdown-trace.sh %{buildroot}%{_libexecdir}/senemos-uke/uke-shutdown-trace
install -Dm644 src/units/uke-shutdown-trace.service %{buildroot}%{_unitdir}/uke-shutdown-trace.service
install -Dm644 src/units/30-uke-power-profiles-home.conf %{buildroot}%{_unitdir}/power-profiles-daemon.service.d/30-uke-state-home.conf
install -Dm755 uke-boot-status %{buildroot}/usr/bin/uke-boot-status
install -Dm755 uke-native-power-profile-ready %{buildroot}/usr/libexec/senemos-uke/uke-native-power-profile-ready
install -Dm644 src/units/uke-native-power-profile-ready.service %{buildroot}/usr/lib/systemd/system/uke-native-power-profile-ready.service
install -Dm644 src/profiles/72-uke-native-power-profile.rules %{buildroot}/usr/lib/udev/rules.d/72-uke-native-power-profile.rules
install -Dm755 uke-usb-port %{buildroot}/usr/bin/uke-usb-port
install -Dm644 src/units/uke-usb-observer.service %{buildroot}/usr/lib/systemd/system/uke-usb-observer.service
install -Dm644 src/profiles/org.senemos.Uke.USB1.conf %{buildroot}/usr/share/dbus-1/system.d/org.senemos.Uke.USB1.conf
install -Dm755 uke-wifi-factory %{buildroot}/usr/libexec/senemos-uke/uke-wifi-factory
install -Dm644 src/units/uke-wifi-factory.service %{buildroot}/usr/lib/systemd/system/uke-wifi-factory.service
install -Dm755 uke-bluetooth-address %{buildroot}/usr/libexec/uke-bluetooth-address
install -Dm644 packaging/uke-bluetooth-address.service %{buildroot}/usr/lib/systemd/system/uke-bluetooth-address.service
install -Dm644 packaging/70-uke-bluetooth.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-bluetooth.rules
install -d %{buildroot}/usr/share/senemos/boot/uke
install -m644 src/profiles/* %{buildroot}/usr/share/senemos/boot/uke/
install -Dm644 src/units/uke-cdc-acm.service %{buildroot}/usr/lib/systemd/system/uke-cdc-acm.service
install -Dm644 src/units/uke-cdc-shell.service %{buildroot}/usr/lib/systemd/system/uke-cdc-shell.service
install -Dm755 src/uke-usb-network.sh %{buildroot}/usr/libexec/senemos-uke/uke-usb-network
# UKE_USB_RECOVERY31_INSTALL_BEGIN
overlay=%{_builddir}/uke-usb-recovery31
install -Dm644 "$overlay/src/units/uke-usb-recovery.service" %{buildroot}%{_unitdir}/uke-usb-recovery.service
install -d %{buildroot}%{_unitdir}/multi-user.target.wants
ln -s ../uke-usb-recovery.service %{buildroot}%{_unitdir}/multi-user.target.wants/uke-usb-recovery.service
install -Dm644 "$overlay/src/profiles/92-uke-usb-recovery.rules" %{buildroot}%{_udevrulesdir}/92-uke-usb-recovery.rules
install -Dm600 "$overlay/src/profiles/uke-usb-recovery.nmconnection" %{buildroot}/usr/lib/NetworkManager/system-connections/uke-usb-recovery.nmconnection
install -Dm644 "$overlay/src/profiles/uke-recovery.xml" %{buildroot}/usr/lib/firewalld/zones/uke-recovery.xml
install -d "%{buildroot}%{_unitdir}/uke-root-capacity.service.d"
printf '%%s\n' '[Service]' 'StandardOutput=journal' 'StandardError=journal' > "%{buildroot}%{_unitdir}/uke-root-capacity.service.d/20-journal.conf"
install -Dm644 "$overlay/src/profiles/92-uke-usb-recovery.conf" %{buildroot}/usr/lib/NetworkManager/conf.d/92-uke-usb-recovery.conf
# UKE_USB_RECOVERY31_INSTALL_END
install -Dm644 src/profiles/usb-sharing %{buildroot}/etc/senemos/usb-sharing
install -Dm644 src/profiles/80-uke-usb.preset %{buildroot}/usr/lib/systemd/system-preset/80-uke-usb.preset
install -Dm644 src/units/uke-usb-network.service %{buildroot}/usr/lib/systemd/system/uke-usb-network.service
install -Dm644 src/units/uke-usb-network-shell.service %{buildroot}/usr/lib/systemd/system/uke-usb-network-shell.service
install -Dm644 src/profiles/uke-coresight-bringup.conf %{buildroot}/usr/lib/modprobe.d/uke-coresight-bringup.conf
install -Dm755 uke-esp32-cdc %{buildroot}/usr/libexec/senemos-uke/uke-esp32-cdc
install -Dm644 src/units/uke-esp32-cdc-log.service %{buildroot}/usr/lib/systemd/system/uke-esp32-cdc-log.service
install -Dm755 uke-reboot %{buildroot}/usr/bin/uke-reboot
install -Dm755 uke-kernel-update %{buildroot}/usr/bin/uke-kernel-update
install -Dm644 src/profiles/rollback.conf %{buildroot}/etc/senemos/rollback.conf
install -Dm644 src/profiles/kernel-update.conf %{buildroot}/etc/senemos/kernel-update.conf
install -Dm644 src/profiles/userdata-update.conf %{buildroot}/etc/senemos/userdata-update.conf
install -Dm644 src/profiles/80-uke-boot.preset %{buildroot}/usr/lib/systemd/system-preset/80-uke-boot.preset
install -Dm644 src/units/uke-userdata-accept.service %{buildroot}/usr/lib/systemd/system/uke-userdata-accept.service
install -Dm644 src/units/uke-userdata-fallback.service %{buildroot}/usr/lib/systemd/system/uke-userdata-fallback.service
install -Dm755 uke-install-profile %{buildroot}/usr/libexec/senemos-uke/uke-install-profile
install -Dm644 src/units/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
# UKE_BOOT_STARTUP26_CLOCK_BEGIN
install -m644 %{_builddir}/uke-control-boot-startup/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
# UKE_BOOT_STARTUP26_CLOCK_END
install -Dm644 src/units/uke-install-profile.service %{buildroot}/usr/lib/systemd/system/uke-install-profile.service
install -Dm644 src/units/uke-root-grow.service %{buildroot}/usr/lib/systemd/system/uke-root-grow.service
install -Dm644 src/units/uke-rtc-sync.service %{buildroot}/usr/lib/systemd/system/uke-rtc-sync.service
install -Dm644 src/units/uke-rtc-sync.timer %{buildroot}/usr/lib/systemd/system/uke-rtc-sync.timer
install -Dm755 uke-boot-guard %{buildroot}/usr/libexec/senemos-uke/uke-boot-guard
install -Dm644 src/units/uke-boot-guard.service %{buildroot}/usr/lib/systemd/system/uke-boot-guard.service
install -Dm644 src/profiles/uke-reboot.conf %{buildroot}/usr/lib/modules-load.d/uke-reboot.conf
install -Dm755 src/dracut/93uke-boot-guard/module-setup.sh %{buildroot}/usr/lib/dracut/modules.d/93uke-boot-guard/module-setup.sh
install -d %{buildroot}/usr/lib/systemd/system/sysinit.target.wants %{buildroot}/usr/local/{bin,sbin}
ln -s ../uke-boot-guard.service %{buildroot}/usr/lib/systemd/system/sysinit.target.wants/uke-boot-guard.service
ln -s ../../../usr/bin/uke-reboot %{buildroot}/usr/local/bin/reboot
ln -s ../../../usr/bin/uke-reboot %{buildroot}/usr/local/sbin/reboot


)

install -Dm644 %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/src/units/uke-rtc-counter-sync.service %{buildroot}/usr/lib/systemd/system/uke-rtc-counter-sync.service
install -Dm644 %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/src/units/uke-rtc-counter-sync.timer %{buildroot}/usr/lib/systemd/system/uke-rtc-counter-sync.timer

(
mkdir -p '%{_builddir}/uke-control-2.8.1/o82-build/'
cd '%{_builddir}/uke-control-2.8.1/o82-build/'
install -Dpm755 uke-o82-color-profile %{buildroot}%{_bindir}/uke-o82-color-profile

)

if test -d '%{buildroot}/usr/bin/uke-reboot' && ! test -L '%{buildroot}/usr/bin/uke-reboot'; then find '%{buildroot}/usr/bin/uke-reboot' -depth -delete; else rm -f -- '%{buildroot}/usr/bin/uke-reboot'; fi
if test -d '%{buildroot}/usr/lib/dracut/modules.d/93uke-boot-guard' && ! test -L '%{buildroot}/usr/lib/dracut/modules.d/93uke-boot-guard'; then find '%{buildroot}/usr/lib/dracut/modules.d/93uke-boot-guard' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/dracut/modules.d/93uke-boot-guard'; fi
if test -d '%{buildroot}/usr/lib/modules-load.d/uke-reboot.conf' && ! test -L '%{buildroot}/usr/lib/modules-load.d/uke-reboot.conf'; then find '%{buildroot}/usr/lib/modules-load.d/uke-reboot.conf' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/modules-load.d/uke-reboot.conf'; fi
if test -d '%{buildroot}/usr/lib/systemd/system/sysinit.target.wants/uke-boot-guard.service' && ! test -L '%{buildroot}/usr/lib/systemd/system/sysinit.target.wants/uke-boot-guard.service'; then find '%{buildroot}/usr/lib/systemd/system/sysinit.target.wants/uke-boot-guard.service' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/systemd/system/sysinit.target.wants/uke-boot-guard.service'; fi
if test -d '%{buildroot}/usr/lib/systemd/system/uke-boot-guard.service' && ! test -L '%{buildroot}/usr/lib/systemd/system/uke-boot-guard.service'; then find '%{buildroot}/usr/lib/systemd/system/uke-boot-guard.service' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/systemd/system/uke-boot-guard.service'; fi
if test -d '%{buildroot}/usr/lib/systemd/system/uke-cdc-shell.service' && ! test -L '%{buildroot}/usr/lib/systemd/system/uke-cdc-shell.service'; then find '%{buildroot}/usr/lib/systemd/system/uke-cdc-shell.service' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/systemd/system/uke-cdc-shell.service'; fi
if test -d '%{buildroot}/usr/lib/systemd/system/uke-esp32-cdc-log.service' && ! test -L '%{buildroot}/usr/lib/systemd/system/uke-esp32-cdc-log.service'; then find '%{buildroot}/usr/lib/systemd/system/uke-esp32-cdc-log.service' -depth -delete; else rm -f -- '%{buildroot}/usr/lib/systemd/system/uke-esp32-cdc-log.service'; fi
if test -d '%{buildroot}/usr/libexec/senemos-uke/uke-boot-guard' && ! test -L '%{buildroot}/usr/libexec/senemos-uke/uke-boot-guard'; then find '%{buildroot}/usr/libexec/senemos-uke/uke-boot-guard' -depth -delete; else rm -f -- '%{buildroot}/usr/libexec/senemos-uke/uke-boot-guard'; fi
if test -d '%{buildroot}/usr/libexec/senemos-uke/uke-esp32-cdc' && ! test -L '%{buildroot}/usr/libexec/senemos-uke/uke-esp32-cdc'; then find '%{buildroot}/usr/libexec/senemos-uke/uke-esp32-cdc' -depth -delete; else rm -f -- '%{buildroot}/usr/libexec/senemos-uke/uke-esp32-cdc'; fi
if test -d '%{buildroot}/usr/local/bin/reboot' && ! test -L '%{buildroot}/usr/local/bin/reboot'; then find '%{buildroot}/usr/local/bin/reboot' -depth -delete; else rm -f -- '%{buildroot}/usr/local/bin/reboot'; fi
if test -d '%{buildroot}/usr/local/sbin/reboot' && ! test -L '%{buildroot}/usr/local/sbin/reboot'; then find '%{buildroot}/usr/local/sbin/reboot' -depth -delete; else rm -f -- '%{buildroot}/usr/local/sbin/reboot'; fi
install -Dm644 %{SOURCE10} %{buildroot}/usr/share/dnf5/libdnf.conf.d/60-uke-filelists.conf

install -Dm755 %{_builddir}/uke-control-session/uke-speaker-orientation %{buildroot}/usr/libexec/uke-speaker-orientation
install -Dm644 %{_builddir}/uke-control-session/packaging/uke-speaker-orientation.desktop %{buildroot}/etc/xdg/autostart/uke-speaker-orientation.desktop
install -Dm644 %{_builddir}/uke-control-session/docs/SPEAKER-ORIENTATION.md %{buildroot}%{_docdir}/uke-control/SPEAKER-ORIENTATION.md
install -Dm644 %{_builddir}/uke-control-boot-health/BOOT-HEALTH-OBSERVATION.md %{buildroot}%{_docdir}/uke-control/BOOT-HEALTH-OBSERVATION.md
install -Dm644 %{_builddir}/uke-control-critical-rollback/CRITICAL-ROLLBACK.md %{buildroot}%{_docdir}/uke-control/CRITICAL-ROLLBACK.md
install -Dm755 %{_builddir}/uke-control-brightness/brightness-curve %{buildroot}/usr/libexec/uke-control/brightness-curve
install -Dm644 %{_builddir}/uke-control-brightness/packaging/30-uke-brightness-curve.sh %{buildroot}/etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh
install -Dm644 %{_builddir}/uke-control-brightness/docs/BRIGHTNESS-CURVE.md %{buildroot}%{_docdir}/uke-control/BRIGHTNESS-CURVE.md
test -x %{buildroot}/usr/libexec/uke-control/brightness-curve
sh -n %{buildroot}/etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh
install -Dm755 %{_builddir}/uke-control-speaker/uke-speaker-runtime %{buildroot}/usr/bin/uke-speaker-runtime
install -Dm755 %{_builddir}/uke-control-speaker/src/audio/speaker-gain-preflight.sh %{buildroot}/usr/bin/uke-speaker-gain-preflight
install -Dm755 %{_builddir}/uke-control-speaker/src/audio/prepare-generic-speaker-data.sh %{buildroot}/usr/libexec/uke-speaker-prepare-data
install -Dm644 %{_builddir}/uke-control-speaker/src/audio/systemd/uke-speaker-provision@.service %{buildroot}/usr/lib/systemd/system/uke-speaker-provision@.service
install -Dm644 %{_builddir}/uke-control-speaker/src/audio/udev/70-uke-speaker-provision.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-speaker-provision.rules
install -Dm644 %{_builddir}/uke-control-speaker-overlay/docs/SPEAKER-RUNTIME-CONSOLIDATION.md %{buildroot}%{_docdir}/uke-control/SPEAKER-RUNTIME-CONSOLIDATION.md
selinux_overlay=%{_builddir}/uke-control-selinux-integration
install -Dm755 "$selinux_overlay/packaging/selinux/selinux-kernel-abi.sh" %{buildroot}/usr/libexec/uke-control/selinux-kernel-abi
install -Dm644 "$selinux_overlay/packaging/selinux/compose-kernel-abi.awk" %{buildroot}/usr/libexec/uke-control/compose-kernel-abi.awk
install -Dm644 "$selinux_overlay/docs/SELINUX-ORIGINAL.md" %{buildroot}%{_docdir}/uke-control/SELINUX-ORIGINAL.md

install -Dm644 %{_builddir}/uke-control-closed-cover-health/uke-boot/docs/CLOSED-COVER-HEALTH.md %{buildroot}%{_docdir}/uke-control/CLOSED-COVER-HEALTH.md

cdsp=%{_builddir}/uke-cdsp-normal-firmware
install -Dm755 "$cdsp/replay/helper" %{buildroot}%{_libexecdir}/uke-cdsp-pogo-firmware
cmp "$cdsp/replay/helper" %{buildroot}%{_libexecdir}/uke-cdsp-pogo-firmware
install -Dm644 "$cdsp/replay/uke-cdsp-pogo-firmware/README.md" %{buildroot}%{_docdir}/uke-cdsp-pogo-firmware/README.md
%check
bash %{_builddir}/uke-control-suspend-dsc-policy/tests/hardware-sleep-policy.sh %{_builddir}/uke-control-suspend-dsc-policy/packaging
(
cd %{_builddir}/uke-control-display-dock
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Isrc tests/dock-layout.cpp src/dock-layout.cpp %{build_ldflags} -o dock-layout
./dock-layout
sed -n '/^bool wayland_socket(/,/^std::optional<std::string> doctor(/p' src/dock-desktop.cpp | sed '$d' > production-endpoint
sed -n '/^bool wayland_socket(/,/^#include <csignal>/p' tests/wayland-manager-test.cpp | sed '$d' > tested-endpoint
cmp production-endpoint tested-endpoint
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror -UNDEBUG $(pkg-config --cflags libsystemd) tests/wayland-manager-test.cpp %{build_ldflags} $(pkg-config --libs libsystemd) -o dock-readiness
runtime=$(mktemp -d /tmp/uke-wayland.XXXXXXXX)
trap 'rmdir -- "$runtime"' EXIT
timeout 20s dbus-run-session -- ./dock-readiness "$runtime"
test "$(readlink %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-control-display-dock.service)" = ../uke-control-display-dock.service
printf 'Uke display dock layout policy and session activation passed.\n'
)
(
cd %{_builddir}/uke-control-boot-notify
%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror -UNDEBUG $(pkg-config --cflags libsystemd) tests/boot-notify-bus.cpp %{build_ldflags} $(pkg-config --libs libsystemd) -pthread -o boot-notify-bus
timeout 20s dbus-run-session -- ./boot-notify-bus
test "$(readlink %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.service)" = ../uke-boot-notify.service
test "$(readlink %{buildroot}%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.path)" = ../uke-boot-notify.path
test "$(stat -c %%a %{buildroot}%{_sharedstatedir}/senemos/kernel-notifications)" = 755
)
artifact=%{buildroot}/usr/bin/uke-control
LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$artifact" display cvt-rb3 3840 2160 119.88 --json | jq -e '.standard == "CVT-RB3" and .clock_khz == 1074729 and .vertical.total == 2287'
LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$artifact" display cvt-rb3 1366 768 59.94 520 16 --json | jq -e '.horizontal.active == 1360 and .horizontal.total == 1456'
rc=0
LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$artifact" display cvt-rb3 1920 1080 nan || rc=$?
test "$rc" = 64
(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
cd "$control"
g++ -O2 -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Isrc tests/plymouth-handoff.cpp -o build/test-plymouth-handoff
build/test-plymouth-handoff
cmp packaging/50-uke-display-handoff.conf %{buildroot}%{_unitdir}/plymouth-quit.service.d/50-uke-display-handoff.conf
cmp packaging/50-uke-display-handoff-wait.conf %{buildroot}%{_unitdir}/plymouth-quit-wait.service.d/50-uke-display-handoff.conf
artifact=%{buildroot}/usr/bin/uke-control
if env -u NOTIFY_SOCKET LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$artifact" plymouth-handoff; then
    exit 1
else
    test "$?" = 1
fi
env -u SERVICE_RESULT LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$artifact" plymouth-handoff-recover
)
LD_LIBRARY_PATH=%{buildroot}%{_libdir} %{buildroot}/usr/bin/uke-control firmware --help | grep -Ex " +uke-control firmware cdsp-probe"
(
test "$(id -u)" != 0
set +e
output=$(LD_LIBRARY_PATH=%{buildroot}%{_libdir} %{buildroot}/usr/bin/uke-control firmware cdsp-probe 2>&1)
result=$?
set -e
test "$result" = 1
grep -Fq "CDSP FastRPC diagnostics require root" <<< "$output"
)
cdsp=%{_builddir}/uke-cdsp-normal-firmware
installed=%{buildroot}%{_libexecdir}/uke-cdsp-pogo-firmware
# RPM buildroot policy removes debug sections before check. Verify the runtime
# image bytes and program header mappings against the exact compiled helper.
objcopy -O binary "$cdsp/replay/helper" "$cdsp/original.load"
objcopy -O binary "$installed" "$cdsp/installed.load"
cmp "$cdsp/original.load" "$cdsp/installed.load"
readelf -Wl "$cdsp/replay/helper" | sed '/Section to Segment mapping:/,$d' > "$cdsp/original.phdr"
readelf -Wl "$installed" | sed '/Section to Segment mapping:/,$d' > "$cdsp/installed.phdr"
cmp "$cdsp/original.phdr" "$cdsp/installed.phdr"
cmp "$cdsp/replay/uke-cdsp-pogo-firmware/README.md" %{buildroot}%{_docdir}/uke-cdsp-pogo-firmware/README.md
grep -F 'Normal ABI source-family checks' "$cdsp/qualification.txt"
test "$(readlink %{buildroot}/usr/bin/ukectl)" = uke-control
# UKE_USB_RECOVERY31_CHECK_BEGIN
(
overlay=%{_builddir}/uke-usb-recovery31
udevadm verify "%{buildroot}%{_udevrulesdir}/92-uke-usb-recovery.rules"
test "$(readlink %{buildroot}%{_unitdir}/multi-user.target.wants/uke-usb-recovery.service)" = ../uke-usb-recovery.service
test "$(stat -c '%%a' %{buildroot}/usr/lib/NetworkManager/system-connections/uke-usb-recovery.nmconnection)" = 600
cmp "$overlay/src/profiles/92-uke-usb-recovery.conf" %{buildroot}/usr/lib/NetworkManager/conf.d/92-uke-usb-recovery.conf
cmp "$overlay/src/profiles/uke-usb-recovery.nmconnection" %{buildroot}/usr/lib/NetworkManager/system-connections/uke-usb-recovery.nmconnection
cmp "$overlay/src/profiles/uke-recovery.xml" %{buildroot}/usr/lib/firewalld/zones/uke-recovery.xml
cmp "$overlay/src/units/uke-usb-recovery.service" %{buildroot}%{_unitdir}/uke-usb-recovery.service
test ! -e %{buildroot}/etc/senemos/usb-recovery
)
printf '%%s\n' '[Service]' 'StandardOutput=journal' 'StandardError=journal' | cmp - "%{buildroot}%{_unitdir}/uke-root-capacity.service.d/20-journal.conf"
# UKE_USB_RECOVERY31_CHECK_END
# UKE_UKECTL29_CHECK_BEGIN
bash %{_builddir}/uke-control-ukectl29/check.sh %{buildroot}
# UKE_UKECTL29_CHECK_END
# UKE_ROOT_CAPACITY54_CHECK_BEGIN
(
set -Eeuo pipefail
test "$(id -u)" != 0
helper=%{buildroot}%{_libexecdir}/uke-control/ukectl
readelf -d "$helper" | grep -F libext2fs.so.
readelf -d "$helper" | grep -F libmount.so.
result=0
output=$(LD_LIBRARY_PATH=%{buildroot}%{_libdir} "$helper" verify-root-capacity 2>&1) || result=$?
test "$result" = 1
grep -Fq "Root privileges required for capacity verification" <<< "$output"
)
# UKE_ROOT_CAPACITY54_CHECK_END
# UKE_SWAP_READY28_CHECK_BEGIN
test "$(grep -c '^PASS: actual swap startup' %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt)" = 4
cmp %{_builddir}/uke-control-swap-ready/results.txt %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt
grep -Fxq 'ExecStart=/usr/bin/uke-control swap ready' %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
! grep -q '^ExecStart=' %{buildroot}/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
# UKE_SWAP_READY28_CHECK_END
# UKE_FACTORY_INPUTS27_CHECK_BEGIN
test "$(grep -c '^PASS: actual factory input:' %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt)" = 8
cmp %{_builddir}/uke-control-factory-inputs/results.txt %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt
# UKE_FACTORY_INPUTS27_CHECK_END
# UKE_BOOT_STARTUP26_CHECK_BEGIN
cmp %{_builddir}/uke-control-boot-startup/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
grep -Fx 'StateDirectory=senemos/installation' %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
grep -Fx 'StateDirectoryMode=0700' %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
for name in boot-lock setup-cdc super-swap;do
 cmp %{_builddir}/uke-control-boot-startup/$name-results.txt %{buildroot}%{_docdir}/uke-control/startup26/$name-results.txt
done
# UKE_BOOT_STARTUP26_CHECK_END
# UKE_SUPER_SWAP25_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(%{buildroot}/usr/libexec/uke-control/super-swap/prepare.sh --prepare 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s\n' "$output" | grep -Fx 'UKE_SUPER_SWAP_REJECTED: root required'
cmp %{_builddir}/uke-control-super-swap/super-swap-results.txt %{buildroot}%{_docdir}/uke-control/super-swap-results.txt
for file in %{_builddir}/uke-control-super-swap/runtime/*.sh;do
 # Fedora brp-mangle-shebangs resolves env bash to the packaged /usr/bin/bash.
 # Admit exactly that first-line transformation, comparing every other byte.
 sed '1s|^#!/usr/bin/env bash$|#!/usr/bin/bash|' "$file" | cmp - "%{buildroot}/usr/libexec/uke-control/super-swap/${file##*/}"
done
)
# UKE_SUPER_SWAP25_CHECK_END
# UKE_SUPER_BOOT24_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(%{buildroot}/usr/bin/uke-kernel-update derive-slot-image /missing /missing a 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s\n' "$output" | grep -Fx 'uke-kernel-update: Root privileges are required'
cmp %{_builddir}/uke-control-super-boot/native-results.txt %{buildroot}%{_docdir}/uke-control/super-boot-native-results.txt
)
# UKE_SUPER_BOOT24_CHECK_END
# UKE_SETUP_CDC23_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(LD_LIBRARY_PATH=%{buildroot}/usr/lib64 %{buildroot}/usr/bin/uke-control setup-cdc apply --json 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s' "$output" | jq -e 'tostring | contains("Setup CDC requires root")' >/dev/null
for unit in uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path;do
 cmp "%{_builddir}/uke-control-setup-cdc/packaging/$unit" "%{buildroot}/usr/lib/systemd/system/$unit"
done
test ! -e %{buildroot}/etc/uke-control/first-setup-cdc
)
# UKE_SETUP_CDC23_CHECK_END
# UKE_BOOT_IDENTITY22_CHECK_BEGIN
bash %{SOURCE73} "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102" %{buildroot}/usr/bin/uke-boot-status
# UKE_BOOT_IDENTITY22_CHECK_END
# UKE_USB_API_OUTPUT_CHECK_BEGIN
printf '%s  %s\n' "$(jq -er .fixture_sha256 %{SOURCE69})" %{SOURCE70} \
 "$(jq -er .parent.runtime_sha256 %{SOURCE69})" %{SOURCE71} | sha256sum -c --strict --quiet -
# UKE_PIDFD30_CHECK_BEGIN
sed -e '/^#include <fcntl.h>$/a #include <limits>' -e '/^#include <stdexcept>$/a #include <sys/syscall.h>' \
    %{SOURCE70} > %{_builddir}/uke-control-pidfd30/rpm-api-output.sh
bash %{_builddir}/uke-control-pidfd30/rpm-api-output.sh "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1" %{SOURCE71}
# UKE_PIDFD30_CHECK_END
# UKE_USB_API_OUTPUT_CHECK_END
(
%set_build_flags
cd %{_builddir}/uke-control-motion/uke-control
make -f packaging/motion.mk CXXFLAGS="$CXXFLAGS -std=c++20 -Wall -Wextra -Werror" LDFLAGS="$LDFLAGS" check
make -f packaging/motion.mk CXX=clang++ motion_out=build/motion-clang CXXFLAGS="-O1 -g -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all" LDFLAGS="-fsanitize=address,undefined" check
UKE_POLICY_OFFLINE_FIXTURE=1 bash tests/motion-policy.sh %{SOURCE65} build/motion/selinux/uke_control_motion.cil packaging/selinux/motion-policy.sh build/motion/policy-result.txt
bash packaging/check-motion-integration.sh "$PWD" %{buildroot} %{_specdir}/uke-control.spec
bash tests/motion-lifecycle.sh %{_specdir}/uke-control.spec
)
bash '%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/closed-cover-health.sh'
bash %{_builddir}/uke-control-selinux-integration/packaging/check-selinux-integration.sh %{_builddir}/uke-control-selinux-integration %{buildroot} %{_specdir}/uke-control.spec
(
qcomtee=%{_builddir}/uke-qcomtee-fedora-sdk
export PKG_CONFIG_PATH="$qcomtee/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$qcomtee%{_libdir}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
bash %{_builddir}/uke-control-pogo-integration/packaging/check-pogo-integration.sh %{_builddir}/uke-control-pogo-integration %{buildroot}
)

bash %{_builddir}/uke-control-fastrpc-provider/tests/ssc-cache-postun.sh %{_specdir}/uke-control.spec
bash %{_builddir}/uke-control-fastrpc-provider/packaging/check-fastrpc-provider.sh %{_builddir}/uke-control-fastrpc-provider %{_builddir}/uke-control-ssc-overlay %{buildroot}
(
ssc=%{_builddir}/uke-control-ssc-overlay
cd "$ssc/legacy"
mkdir contract
for name in hexagonfs hexagonfs_mapped hexagonfs_virt_dir rpcd_builder;do
 gcc -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include \
 -c hexagonrpcd/$name.c -o contract/$name.o
done
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I hexagonrpcd "$ssc/inputs/hexagon-registry.cpp" contract/*.o -o contract/registry
./contract/registry
sed -n '/^static int create_shell_pd(/,/^}/p' hexagonrpcd/rpcd.c > contract/shell-function.h
gcc -std=gnu11 -O2 -Wall -Wextra -Werror -I contract "$ssc/inputs/hexagon-shell.c" -o contract/shell
./contract/shell
sed -n '/^static void \*start_reverse_tunnel(/,/^}/p' hexagonrpcd/rpcd.c > contract/listener-function.h
gcc -std=gnu11 -O2 -Wall -Wextra -Werror -I contract "$ssc/inputs/hexagon-readiness.c" -o contract/readiness
./contract/readiness
mkdir -p src tests
cp "$ssc/inputs/ssc-cache.cpp" src/ssc-cache.cpp
cp "$ssc/inputs/ssc-cache-test.cpp" tests/ssc-cache.cpp
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-cache.cpp -o contract/cache
mkdir contract/cache-fixture
./contract/cache "$PWD/contract/cache-fixture"
)
bash %{_builddir}/uke-control-ssc-overlay/ssc-unit-state-test.sh %{_specdir}/uke-control.spec
bash %{_builddir}/uke-control-ssc-overlay/ssc-refresh-test.sh %{_specdir}/uke-control.spec
(
cd %{_builddir}/uke-control-ssc-overlay/qualified
test ! -e /etc/senemos/kernel-update.conf
reference=%{_builddir}/uke-fastrpc-fedora-reference
export LD_LIBRARY_PATH="$reference/root%{_libdir}"
test "$(rpm -qp --dbpath "$reference/trust" --qf '%%{VERSION}-%%{RELEASE}' %{SOURCE123})" = 1.0.8-1.fc46
test "$(rpm -qp --dbpath "$reference/trust" --qf '%%{SHA256HEADER}' %{SOURCE123})" = "$(jq -er .fedora.rpm_header_sha256 fastrpc-notify.json)"
printf '%%s  %%s\n' "$(jq -er .fedora.listener_elf_sha256 fastrpc-notify.json)" "$reference/root%{_libdir}/libadsp_default_listener.so.1.0.0" | sha256sum -c --strict --quiet -
(cd "$reference/root"; sha256sum -c --strict --quiet "$reference/extracted.sha256")
readelf -h libuke-fastrpc-notify.so | grep -q AArch64
readelf -d "$reference/root%{_libdir}/libadsp_default_listener.so.1.0.0" > original.dynamic
! grep -q SYMBOLIC original.dynamic
objdump -d "$reference/root%{_libdir}/libadsp_default_listener.so.1.0.0" > original.disassembly
grep -Eq 'bl[[:space:]]+[0-9a-f]+ <adsp_default_listener_register@plt>' original.disassembly
grep -Eq 'bl[[:space:]]+[0-9a-f]+ <adsp_default_listener1_register@plt>' original.disassembly
bash fastrpc-notify-test.sh "$PWD/fixture" "$PWD/libuke-fastrpc-notify.so"
printf '%%s  %%s\n' "$(jq -er .fedora.library_elf_sha256 files/fastrpc-files.json)" "$reference/root%{_libdir}/libadsprpc.so.1.0.0" | sha256sum -c --strict --quiet -
mkdir files-fixture-root
timeout 15 ./files-fixture "$PWD/files-fixture-root"
test ! -e files-fixture-root
./session-abi
bash session/fastrpc-session-test.sh "$PWD/session-fixture" "$PWD/session-worker" "$PWD/libuke-fastrpc-notify.so" "$PWD/session-mocks.so"
(cd "$reference/root"; sha256sum -c --strict --quiet "$reference/extracted.sha256")
)
adaptive=%{_builddir}/uke-control-adaptive/runtime
ctest --test-dir "$adaptive/redhat-linux-build" --output-on-failure --force-new-ctest-process -j"$RPM_BUILD_NCPUS"
for item in 71-uke-tablet-mode.rules 61-uke-display-ddc.rules 60-block-scheduler.rules;do udevadm verify "$adaptive/packaging/$item";done
(cd "$adaptive";sh tests/ddc-config.sh)
systemd-tmpfiles --dry-run --create "$adaptive/packaging/uke-suspend.conf"
bash %{_builddir}/uke-control-adaptive-overlay/adaptive-unit-state-test.sh %{_specdir}/uke-control.spec
bash %{_builddir}/uke-control-adaptive-overlay/check-adaptive-plugin-deps.sh \
 %{buildroot}/usr/lib64/qt6/plugins/ksystemstats/libuke_adreno.so \
 %{_builddir}/uke-control-adaptive-overlay/adaptive-services.json \
 %{_builddir}/uke-control-adaptive/installed-plugin-elf.requires
printf 'Adaptive17 native plugin,parser,DDC,udev,tmpfiles and transaction checks retained;no physical display acceptance implied.\n'
make -C %{_builddir}/uke-control-battery/runtime CXX="%{__cxx}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" validate test-native
bash %{_builddir}/uke-control-battery/overlay/battery-unit-state-test.sh %{_specdir}/uke-control.spec
printf 'Battery/offline original native checks retained; no physical charger-boot acceptance implied.\n'
bash %{_builddir}/uke-control-charge-wait/check-charge-wait-native.sh %{_builddir}/uke-control-charge/runtime
printf 'Control charge wait: actual helper,same protection interval and retained runtime/config ownership; physical power acceptance remains open.\n'
bash %{_builddir}/uke-control-charge/tests/charge-unit-state.sh %{_specdir}/uke-control.spec
%{_builddir}/uke-control-charge/runtime/build/uke-charge-control --help | grep -F apply-cap80-online
printf 'Control charge ownership: native91 source delta,helper and retained unit/config payload; no electrical or event-monitor parity claim.\n'
udevadm verify --resolve-names=never %{SOURCE42}
cmp %{SOURCE42} %{buildroot}/usr/lib/udev/rules.d/89-uke-native-proximity.rules
cmp %{SOURCE43} %{buildroot}%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md
printf 'Control native proximity selection: versioned event-discovery rule and existing216 cohort retained; no new service or physical acceptance.\n'
udevadm verify --resolve-names=never %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules
bash %{_builddir}/uke-control-nfc/nfc/tests/session-acl.sh
bash %{_builddir}/uke-control-nfc/nfc/tests/check.sh
printf '%%s  %%s\n' "$(jq -er .sha256 %{_builddir}/uke-control-nfc/nfc/uapi-source.json)" %{_builddir}/uke-control-nfc/nfc/include/fm19511.h | sha256sum -c --strict --quiet -
readelf -h %{_builddir}/uke-control-nfc/uke-nfc-tag | grep -E 'Machine:.*(AArch64|X86-64)'
%{_builddir}/uke-control-nfc/uke-nfc-tag --help
set +e
%{_builddir}/uke-control-nfc/uke-nfc-tag read 0xffff 2 > nfc-invalid.stdout 2> nfc-invalid.stderr
nfc_status=$?
set -e
test "$nfc_status" = 2
test ! -s nfc-invalid.stdout
grep -F 'without address wrap' nfc-invalid.stderr
cmp %{_builddir}/uke-control-nfc/nfc/70-uke-nfc-ir.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
for compiler in g++ clang++;do
 CXX=$compiler bash %{_builddir}/uke-control-speaker/tests/audio-speaker-runtime-contract.sh
 CXX=$compiler bash %{_builddir}/uke-control-speaker/tests/audio-speaker-service-contract.sh
done
bash %{_builddir}/uke-control-speaker/tests/audio-speaker-preflight-contract.sh
bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-state.sh %{_specdir}/uke-control.spec
bash %{_builddir}/uke-control-session/tests/session/speaker-orientation.sh
test -x %{buildroot}/usr/libexec/uke-speaker-orientation
mapfile -t rollback_sources < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/tests/rollback.cpp')
test ${#rollback_sources[@]} = 1
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror "${rollback_sources[0]}" -o %{_builddir}/rollback-cli-contract
%{_builddir}/rollback-cli-contract
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
(
cd "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
ulimit -c 0
/usr/bin/make -O -j${RPM_BUILD_NCPUS} V=1 VERBOSE=1 check


)

)
contract=%{_builddir}/uke-control-frontend-contract
export LD_LIBRARY_PATH=$(cat "$contract/library-directory")${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
"$contract/frontend-request" --help > /dev/null
set +e
"$contract/frontend-request" status > "$contract/captured-read-refusal" 2>&1
status=$?
set -e
test $status = 64
grep -F 'read-only query interface' "$contract/captured-read-refusal"

mapfile -t seamless_sources < <(find %{_builddir} -type f -path '*/tests/seamless-policy.cpp')
test ${#seamless_sources[@]} = 1
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function "${seamless_sources[0]}" -o %{_builddir}/seamless-policy-contract $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
%{_builddir}/seamless-policy-contract
mapfile -t policy_sources < <(find %{_builddir} -type f -path '*/tests/rollback-policy.cpp')
test ${#policy_sources[@]} = 1
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function "${policy_sources[0]}" -o %{_builddir}/rollback-policy-contract $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
%{_builddir}/rollback-policy-contract
(
cd "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102"
test -f %{buildroot}/usr/lib/clock-epoch
test $(stat -c %Y %{buildroot}/usr/lib/clock-epoch) -eq $(stat -c %Y src/profiles/clock-epoch)
bash tests/oem-video.sh
udevadm verify src/udev/72-uke-system-heap.rules
cmp src/profiles/60-uke-security.conf %{buildroot}/usr/lib/sysctl.d/60-uke-security.conf
bash tests/shutdown-trace.sh
bash tests/ppd-home.sh
bash tests/installation-policy.sh
bash tests/slot-control.sh
bash tests/alternating-prewrite.sh
bash tests/rollback-off-crc.sh
bash tests/crc-snapshot-identity.sh
bash tests/userdata-update.sh
bash %{SOURCE134} src/kernel-update.cpp
bash tests/development-trial.sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-health.cpp -o touch-health-contract
./touch-health-contract
g++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer tests/touch-health.cpp -o touch-health-sanitized
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./touch-health-sanitized
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/usb-port.cpp -o usb-port-contract
./usb-port-contract
bash tests/usb-inventory.sh
bash tests/usb-auto.sh
bash tests/usb-ownership.sh
g++ -std=c++17 -Wall -Wextra -Werror -O2 tests/wifi-factory.cpp -o wifi-factory-contract
./wifi-factory-contract
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-function tests/bluetooth-address.cpp -o bluetooth-address-contract
./bluetooth-address-contract
bash tests/startup-contract.sh
bash tests/startup-native.sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/kernel-update.cpp -o kernel-update-contract $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
./kernel-update-contract | tee merged-kernel-layout-results.txt
grep -Fx 'Merged kernel ownership: exact ABI, uniform layout and preserved rollback checks passed' merged-kernel-layout-results.txt
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/slot-selection.cpp -o slot-selection-contract
./slot-selection-contract
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/rtc-clock.cpp -o rtc-clock-contract
./rtc-clock-contract
bash tests/rtc-packaging.sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/trial-health.cpp -o trial-health-contract
./trial-health-contract
bash tests/automatic-policy-rpm.sh "$PWD/kernel-update-contract"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread tests/native-profile-ready.cpp -o native-profile-ready-contract $(pkg-config --cflags --libs libsystemd)
bash tests/native-profile-ready.sh ./native-profile-ready-contract
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/native-profile-udev.cpp -o native-profile-udev-contract $(pkg-config --cflags --libs umockdev-1.0)
timeout --signal=KILL 25 umockdev-wrapper ./native-profile-udev-contract "$PWD/tests/native-profile-udev.sh"
g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-launcher.cpp -o touch-launcher-contract
./touch-launcher-contract
g++ -std=c++20 -O2 -Wall -Wextra -Werror touch-runtime/legacy/tests/touch-input.cpp touch-runtime/legacy/src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -lsystemd -o touch-input-contract
./touch-input-contract
g++ -std=c++20 -O2 -Wall -Wextra -Werror touch-runtime/legacy/tests/touch-normal-probe.cpp -lcrypto -o touch-normal-contract
./touch-normal-contract
g++ -std=c++20 -O2 -Wall -Wextra -Werror touch-runtime/legacy/tests/touch-assignment.cpp -o touch-assignment-contract
timeout 15 bash -c 'ulimit -v 262144; exec ./touch-assignment-contract'

)
bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/rtc-counter-offset.sh
bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/o82-private-color.sh
bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/managed-firmware.sh

(
mkdir -p '%{_builddir}/uke-control-2.8.1/o82-build/'
cd '%{_builddir}/uke-control-2.8.1/o82-build/'
gcc -std=c11 -Wall -Wextra -Werror -x c -fsyntax-only o82-regdma.h
gcc -std=c11 -Wall -Wextra -Werror -x c -fsyntax-only o82-color-firmware.h
gcc -std=c11 -Wall -Wextra -Werror -x c -fsyntax-only o82-sixzone-format.h
gcc -std=c11 -Wall -Wextra -Werror -x c -fsyntax-only o82-hsic-format.h
./uke-o82-color-profile --self-test
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    o82-color-profile.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) \
    -o color-profile-sanitized
./color-profile-sanitized --self-test
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    -Wl,--wrap=main o82-stock-profile-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o stock-profile-contract
./stock-profile-contract
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -I. \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -Wl,--wrap=main o82-stock-profile-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o stock-profile-sanitized
./stock-profile-sanitized
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    o82-hdr-edid-contract.cpp $(pkg-config --cflags --libs libdisplay-info) -o edid-contract
./edid-contract
bash tests/o82-profile-loader-contract.sh .
bash tests/o82-memory-payload-contract.sh .
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    -Wl,--wrap=main o82-hsic-payload-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o hsic-payload
./hsic-payload
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -I. \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -Wl,--wrap=main o82-hsic-payload-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o hsic-payload-sanitized
./hsic-payload-sanitized
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. o82-hsic-contract.cpp -o hsic-converter
./hsic-converter
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -I. \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    o82-hsic-contract.cpp -o hsic-converter-sanitized
./hsic-converter-sanitized

g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    -Wl,--wrap=main o82-sixzone-payload-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o sixzone-payload
./sixzone-payload
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -I. \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -Wl,--wrap=main o82-sixzone-payload-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o sixzone-payload-sanitized
./sixzone-payload-sanitized
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    o82-sixzone-contract.cpp -o sixzone-converter
./sixzone-converter
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    -Wl,--wrap=main o82-gamut-grid-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o gamut-grid
./gamut-grid
clang++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -I. \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -Wl,--wrap=main o82-gamut-grid-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o gamut-grid-sanitized
./gamut-grid-sanitized
g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I. \
    -DO82_COLOR_IMPLEMENTATION='"o82-color-before-grid.cpp"' \
    -Wl,--wrap=main o82-gamut-grid-contract.cpp $(pkg-config --cflags --libs json-c lcms2 openssl) -o gamut-grid-before
set +e
./gamut-grid-before > gamut-grid-before.log 2>&1
before_status=$?
set -e
test "$before_status" = 1
grep -E '^OEM gamut grid regression rejected: [1-9][0-9]*/[0-9]+ checks failed$' gamut-grid-before.log

)

bash %{SOURCE15} %{SOURCE16}
for compiler in g++ clang++;do
 $compiler -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I%{_sourcedir} %{SOURCE20} -o unstable-release-test
 ./unstable-release-test
done

%post -n uke-control
# UKE_SETUP_CDC23_POST_BEGIN
%systemd_post uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path
# UKE_SETUP_CDC23_POST_END
%systemd_post uke-pogo-auth.service
%systemd_user_post uke-pogo-desktop.service
%udev_rules_update
%systemd_post uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service
%systemd_post uke-adaptive-display.target
command -v systemd-tmpfiles >/dev/null && systemd-tmpfiles --create /usr/lib/tmpfiles.d/uke-suspend.conf || :
%systemd_post uke-battery.service
%systemd_post uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service
(
:
/usr/bin/systemd-sysusers /usr/lib/sysusers.d/uke-control-rgb.conf || exit 1
/usr/bin/semodule -i /usr/share/selinux/packages/uke_control_rgb.pp || exit 1
/usr/bin/restorecon /usr/libexec/uke-control/rgb-led || exit 1

 
if [ $1 -eq 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Initial installation 
    /usr/lib/systemd/systemd-update-helper install-system-units uke-control.service uke-control-wake.service uke-control-cdc.service || : 
fi 

if /usr/bin/systemctl -q is-active dbus.service; then
    /usr/bin/busctl --system call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig >/dev/null || exit 1
fi
/usr/bin/udevadm control --reload >/dev/null 2>&1 || :
/usr/bin/systemctl daemon-reload >/dev/null 2>&1 || :
/usr/libexec/uke-control/torch-access || exit 1
/usr/bin/udevadm trigger --action=add --subsystem-match=leds --sysname-match=uke:white:torch >/dev/null 2>&1 || :
/usr/bin/udevadm trigger --action=add --subsystem-match=misc --sysname-match=uke-rgb >/dev/null 2>&1 || :

 
if [ $1 -eq 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Initial installation 
    /usr/lib/systemd/systemd-update-helper install-user-units uke-control-moisture-notify.service || : 
fi 

) || exit $?
(
:

 
if [ $1 -eq 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Initial installation 
    /usr/lib/systemd/systemd-update-helper install-system-units uke-touchscreen.service || : 
fi 


 
[ -x /usr/lib/systemd/systemd-sysctl ] && /usr/lib/systemd/systemd-sysctl --prefix=/kernel/kptr_restrict --prefix=/kernel/dmesg_restrict || : 

/usr/bin/udevadm control --reload >/dev/null 2>&1 || :

) || exit $?
(
:
if [ -d /run/systemd/system ] && [ -r /sys/firmware/devicetree/base/compatible ]; then
    /usr/bin/uke-kernel-update enable-seamless-policy || exit 1
    /usr/bin/uke-kernel-update migrate-automatic-policy || exit 1
fi

 
if [ $1 -eq 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Initial installation 
    /usr/lib/systemd/systemd-update-helper install-system-units uke-userdata-accept.service || : 
fi 


) || exit $?
(
:

 
if [ $1 -eq 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Initial installation 
    /usr/lib/systemd/systemd-update-helper install-system-units uke-root-grow.service uke-rtc-sync.service uke-rtc-sync.timer || : 
fi 


) || exit $?

# UKE_CONTROL_MOTION_POLICY_INSTALL_BEGIN
/usr/libexec/uke-control/motion-policy install || exit $?
if /usr/bin/selinuxenabled;then /usr/bin/restorecon /usr/libexec/uke-control/motion || exit $?;fi
# UKE_CONTROL_MOTION_POLICY_INSTALL_END

%posttrans -n uke-control
(
# UKE_CONTROL_POGO_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in Control RPM pre/posttrans. No hardware or TEE operation.
set -eu
export LC_ALL=C
state_dir=/run/uke-control-pogo-migration
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_active() {
 case "$1" in active|inactive|failed|activating|deactivating|reloading) return 0;; *) return 1;; esac
}
control() {
 scope=$1;shift
 case "$scope" in
  system) /usr/bin/systemctl "$@" uke-pogo-auth.service;;
  user-*) session_uid=${scope#user-};case "$session_uid" in ''|*[!0-9]*) return 1;;esac
   /usr/bin/systemctl --user --machine="$session_uid@.host" "$@" uke-pogo-desktop.service;;
  *) return 1;;
 esac
}
private_directory() {
 [ -d "$state_dir" ] && [ ! -L "$state_dir" ] &&
 [ "$(/usr/bin/stat -c %%u "$state_dir")" = 0 ] &&
 [ "$(/usr/bin/stat -c %%a "$state_dir")" = 700 ]
}
snapshot() {
 target=$1
 enabled=$(control "$target" is-enabled 2>/dev/null) || :
 active=$(control "$target" is-active 2>/dev/null) || :
 valid_enabled "$enabled" && valid_active "$active" || return 1
 (umask 077;printf '%%s\n%%s\n' "$enabled" "$active" > "$state_dir/$target")
}
known_owner() {
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$1" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do
  case "$owner" in "$2"|uke-control) :;; *) return 1;;esac
 done <<OWNERS
$owners
OWNERS
}
case "$phase" in
 save-upgrade|restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  [ "$1" -gt 0 ] || exit 0
  if [ "$phase" = save-upgrade ];then
   if [ -e "$state_dir" ] || [ -L "$state_dir" ];then
    private_directory && [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
     [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
    # A newer provider's pre already captured the pre-transaction preference.
    exit 0
   fi
   phase=save
  else phase=restore;fi
 ;;
esac
case "$phase" in
 save)
  auth=no;desktop=no
  if known_owner /usr/libexec/uke-pogo-auth uke-pogo-auth;then auth=yes;fi
  if known_owner /usr/libexec/uke-pogo-desktop uke-pogo-desktop;then desktop=yes;fi
  [ "$auth" = yes ] || [ "$desktop" = yes ] || exit 0
  [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
  /usr/bin/mkdir -m700 "$state_dir"
  [ "$auth" != yes ] || snapshot system
  if [ "$desktop" = yes ];then
   for session_dir in /run/user/*;do
    [ -d "$session_dir" ] && [ ! -L "$session_dir" ] && [ -S "$session_dir/bus" ] || continue
    session_uid=${session_dir##*/};case "$session_uid" in ''|*[!0-9]*) continue;;esac
    snapshot "user-$session_uid"
   done
  fi
  (umask 077;printf '1\n' > "$state_dir/complete")
  ;;
 restore)
  [ -e "$state_dir" ] || [ -L "$state_dir" ] || exit 0
  private_directory || exit 1
  [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
   [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
  /usr/bin/systemctl daemon-reload || exit 1
  for entry in "$state_dir"/*;do
   [ "$entry" != "$state_dir/complete" ] || continue
   [ -f "$entry" ] && [ ! -L "$entry" ] && [ "$(/usr/bin/stat -c %%u "$entry")" = 0 ] &&
    [ "$(/usr/bin/stat -c %%a "$entry")" = 600 ] || exit 1
   target=${entry##*/}
   case "$target" in
    system) ;;
    user-*) session_uid=${target#user-};case "$session_uid" in ''|*[!0-9]*) exit 1;;esac
     # A session which logged out cannot be restarted by a package transaction.
     [ -S "/run/user/$session_uid/bus" ] || { /usr/bin/unlink "$entry";continue; }
     /usr/bin/systemctl --user --machine="$session_uid@.host" daemon-reload || exit 1;;
    *) exit 1;;
   esac
   [ "$(/usr/bin/wc -l < "$entry")" = 2 ] || exit 1
   enabled=$(/usr/bin/sed -n '1p' "$entry")
   active=$(/usr/bin/sed -n '2p' "$entry")
   valid_enabled "$enabled" && valid_active "$active" || exit 1
   current=$(control "$target" is-enabled 2>/dev/null) || :
   valid_enabled "$current" || exit 1
   case "$enabled:$current" in *masked*) /usr/bin/unlink "$entry";continue;;esac
   case "$enabled" in
    enabled) control "$target" enable || exit 1;;
    enabled-runtime) control "$target" enable --runtime || exit 1;;
   esac
   case "$active" in active|activating|reloading)
    running=$(control "$target" is-active 2>/dev/null) || :
    case "$running" in
     active|activating|reloading) control "$target" try-restart || exit 1;;
     inactive|failed) control "$target" start || exit 1;;
     deactivating) :;;
     *) exit 1;;
    esac
   ;;esac
   /usr/bin/unlink "$entry"
  done
  /usr/bin/unlink "$state_dir/complete"
  /usr/bin/rmdir "$state_dir"
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_POGO_RESTORE_END
) || exit $?
(
# UKE_CONTROL_SSC_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-ssc-migration
units='uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-ssc-cache.service|uke-ssc-root.service|uke-ssc-sensors.service|iio-sensor-proxy.service|uke-adaptive-display.target) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-ssc-fileservice 2>/dev/null) || old=''
    if [ "$old" != uke-ssc-fileservice ];then
        old=$(/usr/bin/rpm -qf --qf '%%{NAME}' /usr/lib/systemd/system/uke-ssc-root.service 2>/dev/null) || exit 0
        [ "$old" = uke-control ] || exit 0
    fi
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^((uke-ssc-(cache|root|sensors)|iio-sensor-proxy)[.]service|uke-adaptive-display[.]target)$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=6 || n!=5)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_SSC_RESTORE_END
) || exit $?
(
# UKE_CONTROL_ADAPTIVE_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-adaptive-migration
units='uke-adaptive-display.target'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-adaptive-display.target) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-adaptive-display 2>/dev/null) || exit 0
    [ "$old" = uke-adaptive-display ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-adaptive-display[.]target$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=2 || n!=1)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_ADAPTIVE_RESTORE_END
) || exit $?
(
# UKE_CONTROL_BATTERY_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-battery-migration
units='uke-battery.service'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-battery.service) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-battery-observer 2>/dev/null) || exit 0
    [ "$old" = uke-battery-observer ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-battery[.]service$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=2 || n!=1)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_BATTERY_RESTORE_END
) || exit $?
(
# UKE_CONTROL_CHARGE_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-charge-migration
units='uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-charge-control.service|uke-charge-automatic.service|uke-charge-protection.service) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-charge-control 2>/dev/null) || exit 0
    [ "$old" = uke-charge-control ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-charge-(control|automatic|protection)[.]service$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=4 || n!=3)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_CHARGE_RESTORE_END
) || exit $?
# UKE_CONTROL_NFC_RULE_RELOAD
if test -d /run/udev;then
 /usr/bin/udevadm control --reload-rules >/dev/null 2>&1 || :
 /usr/bin/udevadm trigger --action=change --subsystem-match=misc --sysname-match=fm-ntag >/dev/null 2>&1 || :
 /usr/bin/udevadm trigger --action=change --subsystem-match=lirc >/dev/null 2>&1 || :
 # Only reclassify native binary proximity; no hardware request or provider restart.
 /usr/bin/udevadm trigger --action=change --subsystem-match=iio --attr-match=name=uke-ssc-proximity >/dev/null 2>&1 || :
fi
(
:
# UKE_CONTROL_SPEAKER_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans, with phase assigned by the recipe.
# No private calibration, audio command, unit trigger or background process.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-speaker-migration
template=uke-speaker-provision@.service
trap 'result=$?; if [ "$result" != 0 ]; then printf "%%s\n" "Uke speaker ownership migration refused; inspect the retained transaction record before retrying." >&2; fi' EXIT
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0

valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    printf '%%s\n' "$1" | /usr/bin/grep -Eq '^uke-speaker-provision@controlC[0-9]{1,3}[.]service$'
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
restore_enabled() {
    unit=$1; enabled=$2
    case "$enabled" in
        enabled) /usr/bin/systemctl --no-reload enable "$unit";;
        enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
        masked|masked-runtime)
            # Removal must not silently unmask a user's explicit preference.
            [ "$(enabled_state "$unit")" = "$enabled" ];;
        disabled|static|indirect) :;;
        *) return 1;;
    esac
}

case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-speaker-runtime 2>/dev/null) || exit 0
    [ "$old" = uke-speaker-runtime ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    template_state=$(enabled_state "$template") || exit 1
    # Read and validate all states before creating a transaction record.
    rows=$(/usr/bin/systemctl list-units --all --type=service --no-legend --plain 'uke-speaker-provision@*.service') || exit 1
    snapshot=''
    count=0
    for unit in $(printf '%%s\n' "$rows" | /usr/bin/awk 'NF {print $1}'); do
        valid_unit "$unit" || exit 1
        count=$((count+1)); [ "$count" -le 16 ] || exit 1
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ]; then
            case "$template_state:$enabled" in *masked*) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$(/usr/bin/mktemp "$state_dir/.units.XXXXXX") || exit 1
    (umask 077; printf 'V1 %%s %%s\n%%s' "$template_state" "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/chmod 600 "$temporary"
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0; exit 1; }
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    record=$state_dir/units
    [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/stat -c %%s "$record")" -le 4096 ] || exit 1
    # Validate the complete record before any systemd mutation. No eval/source.
    /usr/bin/awk '
      NR==1 {if(NF!=3 || $1!="V1" || $2!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || $3!~/^[1-9][0-9]{0,15}$/)bad=1;next}
      {if(NF!=3 || $1!~/^uke-speaker-provision@controlC[0-9]{1,3}[.]service$/ || seen[$1]++ || $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || ($2=="active" && $3~/masked/))bad=1}
      END{if(NR<1 || NR>17)bad=1;exit bad}' "$record" || exit 1
    IFS=' ' read -r version template_state timestamp < "$record"
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    if [ "$template_state" = masked ] || [ "$template_state" = masked-runtime ]; then
        ! /usr/bin/grep -Eq '^[^ ]+ active ' "$record" || exit 1
    fi
    /usr/bin/systemctl daemon-reload
    restore_enabled "$template" "$template_state" || exit 1
    # The service's native --ensure-loaded takes the authenticated control lock
    # and returns without private-source reads/writes in healthy staged states.
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled; do
        restore_enabled "$unit" "$enabled" || exit 1
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_SPEAKER_RESTORE_END
) || exit $?
(
:
# Release the trace that earlier packages started automatically.
if [ -d /run/systemd/system ]; then
    /usr/bin/systemctl --no-block stop uke-shutdown-trace.service || :
fi
(
old=uke-bluetooth; new=uke-boot-integration; unit=uke-bluetooth-address.service; phase=restore
# POSIX shell fragment embedded only in RPM migration scriptlets.
# Restore the admitted state; never enable a previously disabled hardware path.
case "$phase" in
save)
    [ -d /run/systemd/system ] || exit 0
    test "$(rpm -q --qf '%%{NAME}' "$old" 2>/dev/null)" = "$old" || exit 0
    state=/run/uke-rpm-migration
    [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 1
    install -d -m700 "$state"
    enabled=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    active=$(systemctl is-active "$unit" 2>/dev/null) || :
    case "$enabled" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; *) exit 1;; esac
    case "$active" in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac
    (umask 077; printf '%%s\n%%s\n' "$enabled" "$active" >"$state/$new")
    ;;
restore)
    if [ "$new" = uke-iio-sensor-proxy ]; then
        if /usr/bin/selinuxenabled; then
            /usr/bin/semodule -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        else
            /usr/bin/semodule -n -s targeted -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        fi
    fi
    state=/run/uke-rpm-migration
    [ -f "$state/$new" ] && [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 0
    enabled=$(sed -n '1p' "$state/$new")
    active=$(sed -n '2p' "$state/$new")
    current=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    case "$current" in masked|masked-runtime) unlink "$state/$new"; exit 0;; esac
    systemctl daemon-reload || exit 1
    case "$enabled" in
        enabled) systemctl enable "$unit" || exit 1;;
        enabled-runtime) systemctl enable --runtime "$unit" || exit 1;;
        masked|masked-runtime) unlink "$state/$new"; exit 0;;
    esac
    if [ "$active" = active ]; then systemctl start "$unit" || exit 1; fi
    unlink "$state/$new"
    ;;
*) exit 1;;
esac
) || exit 1
(
old=uke-touchscreen; new=uke-boot-integration-touch; unit=uke-touchscreen.service; phase=restore
# POSIX shell fragment embedded only in RPM migration scriptlets.
# Restore the admitted state; never enable a previously disabled hardware path.
case "$phase" in
save)
    [ -d /run/systemd/system ] || exit 0
    test "$(rpm -q --qf '%%{NAME}' "$old" 2>/dev/null)" = "$old" || exit 0
    state=/run/uke-rpm-migration
    [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 1
    install -d -m700 "$state"
    enabled=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    active=$(systemctl is-active "$unit" 2>/dev/null) || :
    case "$enabled" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; *) exit 1;; esac
    case "$active" in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac
    (umask 077; printf '%%s\n%%s\n' "$enabled" "$active" >"$state/$new")
    ;;
restore)
    if [ "$new" = uke-iio-sensor-proxy ]; then
        if /usr/bin/selinuxenabled; then
            /usr/bin/semodule -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        else
            /usr/bin/semodule -n -s targeted -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        fi
    fi
    state=/run/uke-rpm-migration
    [ -f "$state/$new" ] && [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 0
    enabled=$(sed -n '1p' "$state/$new")
    active=$(sed -n '2p' "$state/$new")
    current=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    case "$current" in masked|masked-runtime) unlink "$state/$new"; exit 0;; esac
    systemctl daemon-reload || exit 1
    case "$enabled" in
        enabled) systemctl enable "$unit" || exit 1;;
        enabled-runtime) systemctl enable --runtime "$unit" || exit 1;;
        masked|masked-runtime) unlink "$state/$new"; exit 0;;
    esac
    if [ "$active" = active ]; then systemctl start "$unit" || exit 1; fi
    unlink "$state/$new"
    ;;
*) exit 1;;
esac
) || exit 1

) || exit $?
(
:
if [ -d /run/systemd/system ] && [ -r /sys/firmware/devicetree/base/compatible ]; then
    /usr/libexec/senemos-uke/uke-install-profile reconcile || :
fi

if [ -d /run/systemd/system ] && [ -d /sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100 ] && [ ! -e /sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100/qcom,uefi-rtc-info ]; then
    systemctl daemon-reload || :
    systemctl --no-block start uke-rtc-counter-sync.timer || :
fi
) || exit $?
(
:
# UKE_CONTROL_TRANSFER_RESTORE_BEGIN
state=/run/uke-control-package-migration
if test -f "$state/units"; then
 test ! -L "$state" && test ! -L "$state/units" || exit 1
 test "$(stat -c %%u "$state/units")" = "$(id -u)" || exit 1
 systemctl daemon-reload || exit 1
 while IFS="$(printf '\t')" read -r unit enabled active; do
  case $unit in uke-*.service|uke-*.timer) ;; *) exit 1;; esac
  case $enabled in enabled) systemctl enable "$unit" || exit 1;; enabled-runtime) systemctl enable --runtime "$unit" || exit 1;; masked|masked-runtime) continue;; disabled|static|indirect) ;; *) exit 1;; esac
  case $active in active) systemctl --no-block start "$unit" || exit 1;; inactive|failed|activating|deactivating) ;; *) exit 1;; esac
 done < "$state/units"
 rm -f -- "$state/units"
 rmdir "$state" || :
fi
# UKE_CONTROL_TRANSFER_RESTORE_END
) || exit $?

(
# UKE_CONTROL_MOTION_RESTORE_BEGIN
phase=restore
# SPDX-License-Identifier: MIT
# Embedded package hooks. Only the owned, demand-activated Motion unit changes.
set -eu
export LC_ALL=C
state=/run/uke-control-motion-upgrade
unit=uke-control-motion.service
unit_file=/usr/lib/systemd/system/uke-control-motion.service
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;;*) return 1;;esac
}
owned_unit() {
 [ -f "$unit_file" ] && [ ! -L "$unit_file" ] || return 1
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$unit_file" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do [ "$owner" = uke-control ] || return 1;done <<OWNERS
$owners
OWNERS
}
read_record() {
 [ -d "$state" ] && [ ! -L "$state" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state")" = 0:0:700 ] || return 1
 [ -f "$state/unit" ] && [ ! -L "$state/unit" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$state/unit")" = 0:0:600:1 ] || return 1
 [ "$(/usr/bin/find "$state" -mindepth 1 -maxdepth 1 | /usr/bin/wc -l)" = 1 ] || return 1
 [ "$(/usr/bin/wc -c < "$state/unit")" -le 256 ] &&
 [ "$(/usr/bin/wc -l < "$state/unit")" = 3 ] || return 1
 IFS=' ' read -r version saved extra < "$state/unit"
 [ "$version" = 1 ] && [ -z "$extra" ] || return 1
 case "$saved" in ''|*[!0-9]*) return 1;;esac
 [ "${#saved}" -le 10 ] || return 1
 now=$(/usr/bin/date +%%s) || return 1
 [ "$saved" -le "$now" ] && [ "$((now-saved))" -le 3600 ] || return 1
 enabled=$(/usr/bin/sed -n '2p' "$state/unit")
 active=$(/usr/bin/sed -n '3p' "$state/unit")
 valid_enabled "$enabled" || return 1
 case "$active" in active|inactive|failed) :;;*) return 1;;esac
}
clear_record() { /usr/bin/unlink "$state/unit" || return 1; /usr/bin/rmdir "$state" || return 1; }
refresh_manager() {
 /usr/bin/systemctl daemon-reload || return 1
 /usr/bin/busctl --system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig || return 1
}
case "$phase" in
 save-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;exit 0;fi
  phase=save;;
 restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  # A newer provider restores after its complete transaction, not here.
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;exit 0;fi
  phase=restore;;
esac
case "$phase" in
 save)
  if [ -e "$state" ] || [ -L "$state" ];then exit 1;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then exit 0;fi
  owned_unit || exit 1
  enabled=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
  valid_enabled "$enabled" || exit 1
  case "$active" in active|inactive|failed) :;;*) exit 1;;esac
  /usr/bin/mkdir -m700 "$state" || exit 1
  captured=$(/usr/bin/date +%%s) || exit 1
  (umask 077;printf '1 %%s\n%%s\n%%s\n' "$captured" "$enabled" "$active" > "$state/unit") || exit 1
  read_record || exit 1
  # Stop while the outgoing executable and its recovery action still exist.
  if [ "$active" = active ];then /usr/bin/systemctl stop "$unit" || exit 1;fi
  ;;
 restore)
  has_record=no
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;has_record=yes;fi
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;fi
  # New installs also need the bus activation and policy configuration re-read.
  refresh_manager || exit 1
  if [ "$has_record" = no ];then exit 0;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then clear_record || exit 1;exit 0;fi
  current=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  valid_enabled "$current" || exit 1
  case "$enabled:$current" in *masked*) clear_record || exit 1;exit 0;;esac
  if [ "$active" = active ];then
   running=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
   case "$running" in
    active) :;;
    inactive|failed) /usr/bin/systemctl start "$unit" || exit 1;;
    *) exit 1;;
   esac
  fi
  clear_record || exit 1
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_MOTION_RESTORE_END
) || exit $?

# UKE_USB_RECOVERY31_POSTTRANS_BEGIN
if [ "${UKE_USB_RECOVERY_ON_UPDATE:-}" = disable ] && [ -d /run/systemd/system ]; then
    if /usr/bin/timeout --kill-after=2s 45s /usr/libexec/senemos-uke/uke-usb-network recovery-disable; then
        echo 'USB Ethernet recovery disabled'
    else
        echo 'USB Ethernet recovery disable did not complete; inspect the current policy and gadget' >&2
        exit 1
    fi
elif [ "${UKE_USB_RECOVERY_ON_UPDATE:-}" = enable ] && [ -d /run/systemd/system ]; then
    if /usr/bin/timeout --kill-after=2s 45s /usr/libexec/senemos-uke/uke-usb-network recovery-enable; then
        echo 'USB Ethernet recovery activation scheduled'
    else
        echo 'USB Ethernet recovery activation did not complete; inspect the existing firewall and link state' >&2
    fi
elif [ -d /run/systemd/system ] && [ -f /etc/senemos/usb-recovery ]; then
    if ! /usr/bin/timeout --kill-after=2s 45s /usr/libexec/senemos-uke/uke-usb-network recovery-refresh; then
        echo 'USB Ethernet recovery reconciliation did not complete' >&2
    fi
fi
# UKE_USB_RECOVERY31_POSTTRANS_END
%postun -n uke-control
# UKE_SETUP_CDC23_POSTUN_BEGIN
%systemd_postun uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path
# UKE_SETUP_CDC23_POSTUN_END
# An erase/downgrade must not depend on the outgoing helper still being installed.
# UKE_CONTROL_MOTION_POLICY_ERASE_BEGIN
if [ ! -e /usr/share/selinux/packages/uke_control_motion.cil ];then
/usr/bin/bash -s -- remove <<'UKE_MOTION_POLICY_ERASE'
#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Transaction-only integration; no daemon or hardware access.
set -Eeuo pipefail
export LC_ALL=C
umask 077
root=/
selected=
while (($#));do
    case $1 in
        --root) [[ $# -ge 2 ]];root=$2;shift 2;;
        --profile) [[ $# -ge 2 && $2 =~ ^(targeted|minimum|mls)$ ]];selected=$2;shift 2;;
        install|remove) mode=$1;shift;break;;
        *) echo 'Invalid Motion policy argument' >&2;exit 2;;
    esac
done
[[ $# == 0 && ${mode:-} =~ ^(install|remove)$ ]]
root=$(realpath -e -- "$root")
offline=true
prefix=$root
if [[ $root == / ]];then [[ $EUID == 0 ]];offline=false;prefix=;
else [[ -d $root && -O $root ]];fi
module=uke_control_motion
marker='; uke-control managed Motion1 policy v1'
payload=$prefix/usr/share/selinux/packages/$module.cil
profiles=(targeted minimum mls)
[[ -z $selected ]] || profiles=("$selected")
[[ ! -L $prefix/run ]]
[[ -d $prefix/run ]] || mkdir -m755 -- "$prefix/run"
work=$(mktemp -d "$prefix/run/uke-motion-policy.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
unpack_cil(){
    [[ -f $1 && ! -L $1 ]]
    if [[ $(head -c 3 -- "$1") == BZh ]];then bzip2 -dc -- "$1" > "$2";
    else cat -- "$1" > "$2";fi
}
run_semodule(){ timeout -k 15s 300s semodule -n -p "$root" -s "$profile" "$@"; }
active=
if ! $offline && selinuxenabled;then
    active=$(awk -F= '/^[[:space:]]*SELINUXTYPE[[:space:]]*=/ {v=$2;sub(/[[:space:]]*#.*/,"",v);gsub(/["\047[:space:]]/,"",v);print v}' /etc/selinux/config)
    [[ $active =~ ^(targeted|minimum|mls)$ ]]
fi
if [[ $mode == install ]];then
    [[ -f $payload && ! -L $payload && -O $payload && $(head -n 1 -- "$payload") == "$marker" ]]
fi
for profile in "${profiles[@]}";do
    store=$prefix/var/lib/selinux/$profile
    [[ -f $store/active/modules/100/base/cil ]] || continue
    for path in "$store" "$store/active" "$store/active/modules" "$store/active/modules/400";do [[ ! -L $path ]];done
    run_semodule --list-modules=full > "$work/modules.list"
    if awk -v name="$module" '$2==name&&$1!=400{found=1}END{exit !found}' "$work/modules.list";then
        echo 'Refusing a conflicting Motion policy priority' >&2;exit 1
    fi
    installed=$store/active/modules/400/$module
    disabled=false
    if [[ -e $installed ]];then
        [[ -d $installed && ! -L $installed && -O $installed && -f $installed/lang_ext && ! -L $installed/lang_ext && $(cat "$installed/lang_ext") == cil ]]
        unpack_cil "$installed/cil" "$work/installed.cil"
        [[ $(head -n 1 "$work/installed.cil") == "$marker" ]] || { echo 'Refusing a foreign Motion policy module' >&2;exit 1; }
        if awk -v name="$module" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/modules.list";then disabled=true;fi
    fi
    if [[ $mode == install ]];then
        args=(-X 400 -i "$payload")
        # Preserve an explicit user disable in the same libsemanage transaction.
        if $disabled;then args+=(-d "$module");fi
        run_semodule "${args[@]}"
    elif [[ -d $installed ]];then run_semodule -X 400 -r "$module";
    else continue;fi
    if ! $offline && [[ $profile == "$active" ]];then
        enforcement=$(getenforce)
        timeout -k 15s 300s semodule -p / -s "$profile" -B
        [[ $(getenforce) == "$enforcement" ]]
    fi
done
UKE_MOTION_POLICY_ERASE
motion_policy_status=$?
[ "$motion_policy_status" = 0 ] || exit "$motion_policy_status"
fi
# UKE_CONTROL_MOTION_POLICY_ERASE_END
(
# UKE_CONTROL_MOTION_POSTUN_RESTORE_BEGIN
phase=restore-upgrade
# SPDX-License-Identifier: MIT
# Embedded package hooks. Only the owned, demand-activated Motion unit changes.
set -eu
export LC_ALL=C
state=/run/uke-control-motion-upgrade
unit=uke-control-motion.service
unit_file=/usr/lib/systemd/system/uke-control-motion.service
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;;*) return 1;;esac
}
owned_unit() {
 [ -f "$unit_file" ] && [ ! -L "$unit_file" ] || return 1
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$unit_file" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do [ "$owner" = uke-control ] || return 1;done <<OWNERS
$owners
OWNERS
}
read_record() {
 [ -d "$state" ] && [ ! -L "$state" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state")" = 0:0:700 ] || return 1
 [ -f "$state/unit" ] && [ ! -L "$state/unit" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$state/unit")" = 0:0:600:1 ] || return 1
 [ "$(/usr/bin/find "$state" -mindepth 1 -maxdepth 1 | /usr/bin/wc -l)" = 1 ] || return 1
 [ "$(/usr/bin/wc -c < "$state/unit")" -le 256 ] &&
 [ "$(/usr/bin/wc -l < "$state/unit")" = 3 ] || return 1
 IFS=' ' read -r version saved extra < "$state/unit"
 [ "$version" = 1 ] && [ -z "$extra" ] || return 1
 case "$saved" in ''|*[!0-9]*) return 1;;esac
 [ "${#saved}" -le 10 ] || return 1
 now=$(/usr/bin/date +%%s) || return 1
 [ "$saved" -le "$now" ] && [ "$((now-saved))" -le 3600 ] || return 1
 enabled=$(/usr/bin/sed -n '2p' "$state/unit")
 active=$(/usr/bin/sed -n '3p' "$state/unit")
 valid_enabled "$enabled" || return 1
 case "$active" in active|inactive|failed) :;;*) return 1;;esac
}
clear_record() { /usr/bin/unlink "$state/unit" || return 1; /usr/bin/rmdir "$state" || return 1; }
refresh_manager() {
 /usr/bin/systemctl daemon-reload || return 1
 /usr/bin/busctl --system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig || return 1
}
case "$phase" in
 save-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;exit 0;fi
  phase=save;;
 restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  # A newer provider restores after its complete transaction, not here.
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;exit 0;fi
  phase=restore;;
esac
case "$phase" in
 save)
  if [ -e "$state" ] || [ -L "$state" ];then exit 1;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then exit 0;fi
  owned_unit || exit 1
  enabled=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
  valid_enabled "$enabled" || exit 1
  case "$active" in active|inactive|failed) :;;*) exit 1;;esac
  /usr/bin/mkdir -m700 "$state" || exit 1
  captured=$(/usr/bin/date +%%s) || exit 1
  (umask 077;printf '1 %%s\n%%s\n%%s\n' "$captured" "$enabled" "$active" > "$state/unit") || exit 1
  read_record || exit 1
  # Stop while the outgoing executable and its recovery action still exist.
  if [ "$active" = active ];then /usr/bin/systemctl stop "$unit" || exit 1;fi
  ;;
 restore)
  has_record=no
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;has_record=yes;fi
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;fi
  # New installs also need the bus activation and policy configuration re-read.
  refresh_manager || exit 1
  if [ "$has_record" = no ];then exit 0;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then clear_record || exit 1;exit 0;fi
  current=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  valid_enabled "$current" || exit 1
  case "$enabled:$current" in *masked*) clear_record || exit 1;exit 0;;esac
  if [ "$active" = active ];then
   running=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
   case "$running" in
    active) :;;
    inactive|failed) /usr/bin/systemctl start "$unit" || exit 1;;
    *) exit 1;;
   esac
  fi
  clear_record || exit 1
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_MOTION_POSTUN_RESTORE_END
) || exit $?
%systemd_postun uke-pogo-auth.service
%systemd_user_postun uke-pogo-desktop.service
%udev_rules_update
(
# UKE_CONTROL_POGO_POSTUN_RESTORE_BEGIN
phase=restore-upgrade
# SPDX-License-Identifier: MIT
# Embedded in Control RPM pre/posttrans. No hardware or TEE operation.
set -eu
export LC_ALL=C
state_dir=/run/uke-control-pogo-migration
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_active() {
 case "$1" in active|inactive|failed|activating|deactivating|reloading) return 0;; *) return 1;; esac
}
control() {
 scope=$1;shift
 case "$scope" in
  system) /usr/bin/systemctl "$@" uke-pogo-auth.service;;
  user-*) session_uid=${scope#user-};case "$session_uid" in ''|*[!0-9]*) return 1;;esac
   /usr/bin/systemctl --user --machine="$session_uid@.host" "$@" uke-pogo-desktop.service;;
  *) return 1;;
 esac
}
private_directory() {
 [ -d "$state_dir" ] && [ ! -L "$state_dir" ] &&
 [ "$(/usr/bin/stat -c %%u "$state_dir")" = 0 ] &&
 [ "$(/usr/bin/stat -c %%a "$state_dir")" = 700 ]
}
snapshot() {
 target=$1
 enabled=$(control "$target" is-enabled 2>/dev/null) || :
 active=$(control "$target" is-active 2>/dev/null) || :
 valid_enabled "$enabled" && valid_active "$active" || return 1
 (umask 077;printf '%%s\n%%s\n' "$enabled" "$active" > "$state_dir/$target")
}
known_owner() {
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$1" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do
  case "$owner" in "$2"|uke-control) :;; *) return 1;;esac
 done <<OWNERS
$owners
OWNERS
}
case "$phase" in
 save-upgrade|restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  [ "$1" -gt 0 ] || exit 0
  if [ "$phase" = save-upgrade ];then
   if [ -e "$state_dir" ] || [ -L "$state_dir" ];then
    private_directory && [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
     [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
    # A newer provider's pre already captured the pre-transaction preference.
    exit 0
   fi
   phase=save
  else phase=restore;fi
 ;;
esac
case "$phase" in
 save)
  auth=no;desktop=no
  if known_owner /usr/libexec/uke-pogo-auth uke-pogo-auth;then auth=yes;fi
  if known_owner /usr/libexec/uke-pogo-desktop uke-pogo-desktop;then desktop=yes;fi
  [ "$auth" = yes ] || [ "$desktop" = yes ] || exit 0
  [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
  /usr/bin/mkdir -m700 "$state_dir"
  [ "$auth" != yes ] || snapshot system
  if [ "$desktop" = yes ];then
   for session_dir in /run/user/*;do
    [ -d "$session_dir" ] && [ ! -L "$session_dir" ] && [ -S "$session_dir/bus" ] || continue
    session_uid=${session_dir##*/};case "$session_uid" in ''|*[!0-9]*) continue;;esac
    snapshot "user-$session_uid"
   done
  fi
  (umask 077;printf '1\n' > "$state_dir/complete")
  ;;
 restore)
  [ -e "$state_dir" ] || [ -L "$state_dir" ] || exit 0
  private_directory || exit 1
  [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
   [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
  /usr/bin/systemctl daemon-reload || exit 1
  for entry in "$state_dir"/*;do
   [ "$entry" != "$state_dir/complete" ] || continue
   [ -f "$entry" ] && [ ! -L "$entry" ] && [ "$(/usr/bin/stat -c %%u "$entry")" = 0 ] &&
    [ "$(/usr/bin/stat -c %%a "$entry")" = 600 ] || exit 1
   target=${entry##*/}
   case "$target" in
    system) ;;
    user-*) session_uid=${target#user-};case "$session_uid" in ''|*[!0-9]*) exit 1;;esac
     # A session which logged out cannot be restarted by a package transaction.
     [ -S "/run/user/$session_uid/bus" ] || { /usr/bin/unlink "$entry";continue; }
     /usr/bin/systemctl --user --machine="$session_uid@.host" daemon-reload || exit 1;;
    *) exit 1;;
   esac
   [ "$(/usr/bin/wc -l < "$entry")" = 2 ] || exit 1
   enabled=$(/usr/bin/sed -n '1p' "$entry")
   active=$(/usr/bin/sed -n '2p' "$entry")
   valid_enabled "$enabled" && valid_active "$active" || exit 1
   current=$(control "$target" is-enabled 2>/dev/null) || :
   valid_enabled "$current" || exit 1
   case "$enabled:$current" in *masked*) /usr/bin/unlink "$entry";continue;;esac
   case "$enabled" in
    enabled) control "$target" enable || exit 1;;
    enabled-runtime) control "$target" enable --runtime || exit 1;;
   esac
   case "$active" in active|activating|reloading)
    running=$(control "$target" is-active 2>/dev/null) || :
    case "$running" in
     active|activating|reloading) control "$target" try-restart || exit 1;;
     inactive|failed) control "$target" start || exit 1;;
     deactivating) :;;
     *) exit 1;;
    esac
   ;;esac
   /usr/bin/unlink "$entry"
  done
  /usr/bin/unlink "$state_dir/complete"
  /usr/bin/rmdir "$state_dir"
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_POGO_POSTUN_RESTORE_END
) || exit $?

(
# UKE_FASTRPC_MOUNT_POSTUN_BEGIN
# SPDX-License-Identifier: MIT
# Embedded RPM postun body; source template, never installed as a helper.
# Incoming Control6 captures the five service states in its pre scriptlet.
# Keep a retained provider's warm RAM mount across upgrades and reinstalls.
# If the incoming package no longer owns the mount, stop its readers before
# unmounting; incoming posttrans/refresh restores the captured service states.
if [ -d /run/systemd/system ] &&
   [ ! -f '/usr/lib/systemd/system/run-uke\x2dssc-fedora.mount' ]; then
    pending=''
    for unit in uke-ssc-sensors.service uke-ssc-root.service uke-ssc-cache.service 'run-uke\x2dssc-fedora.mount'; do
        state=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit $?
        case "$state" in
            inactive) :;;
            active|reloading|refreshing|activating|deactivating|maintenance|failed) pending="$pending $unit";;
            *) exit 1;;
        esac
    done
    if [ -n "$pending" ]; then
        set -f
        /usr/bin/systemctl stop $pending || exit $?
    fi
fi
# UKE_FASTRPC_MOUNT_POSTUN_END
) || exit $?
%systemd_postun uke-adaptive-display.target
%systemd_postun uke-battery.service
%systemd_postun uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service
(
:

 
if [ $1 -ge 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package upgrade, not uninstall 
    /usr/lib/systemd/systemd-update-helper mark-restart-system-units uke-control-rgb.service || : 
fi 

if [ "$1" = 0 ]; then
    if [ -c /dev/uke-rgb ] && [ ! -L /dev/uke-rgb ]; then chown root:root /dev/uke-rgb; chmod 0600 /dev/uke-rgb; fi
    /usr/bin/semodule -r uke_control_rgb >/dev/null 2>&1 || :
    /usr/bin/restorecon /dev/uke-rgb >/dev/null 2>&1 || :
fi

 

/usr/bin/udevadm control --reload >/dev/null 2>&1 || :

 
if [ $1 -ge 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package upgrade, not uninstall 
    /usr/lib/systemd/systemd-update-helper mark-restart-user-units uke-control-moisture-notify.service || : 
fi 


) || exit $?
(
:

 
if [ $1 -ge 1 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package upgrade, not uninstall 
    /usr/lib/systemd/systemd-update-helper mark-restart-system-units uke-touchscreen.service || : 
fi 

/usr/bin/udevadm control --reload >/dev/null 2>&1 || :

) || exit $?
(
:

 


) || exit $?
(
:

 


) || exit $?

%pre -n uke-control
(
# UKE_CONTROL_POGO_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in Control RPM pre/posttrans. No hardware or TEE operation.
set -eu
export LC_ALL=C
state_dir=/run/uke-control-pogo-migration
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_active() {
 case "$1" in active|inactive|failed|activating|deactivating|reloading) return 0;; *) return 1;; esac
}
control() {
 scope=$1;shift
 case "$scope" in
  system) /usr/bin/systemctl "$@" uke-pogo-auth.service;;
  user-*) session_uid=${scope#user-};case "$session_uid" in ''|*[!0-9]*) return 1;;esac
   /usr/bin/systemctl --user --machine="$session_uid@.host" "$@" uke-pogo-desktop.service;;
  *) return 1;;
 esac
}
private_directory() {
 [ -d "$state_dir" ] && [ ! -L "$state_dir" ] &&
 [ "$(/usr/bin/stat -c %%u "$state_dir")" = 0 ] &&
 [ "$(/usr/bin/stat -c %%a "$state_dir")" = 700 ]
}
snapshot() {
 target=$1
 enabled=$(control "$target" is-enabled 2>/dev/null) || :
 active=$(control "$target" is-active 2>/dev/null) || :
 valid_enabled "$enabled" && valid_active "$active" || return 1
 (umask 077;printf '%%s\n%%s\n' "$enabled" "$active" > "$state_dir/$target")
}
known_owner() {
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$1" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do
  case "$owner" in "$2"|uke-control) :;; *) return 1;;esac
 done <<OWNERS
$owners
OWNERS
}
case "$phase" in
 save-upgrade|restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  [ "$1" -gt 0 ] || exit 0
  if [ "$phase" = save-upgrade ];then
   if [ -e "$state_dir" ] || [ -L "$state_dir" ];then
    private_directory && [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
     [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
    # A newer provider's pre already captured the pre-transaction preference.
    exit 0
   fi
   phase=save
  else phase=restore;fi
 ;;
esac
case "$phase" in
 save)
  auth=no;desktop=no
  if known_owner /usr/libexec/uke-pogo-auth uke-pogo-auth;then auth=yes;fi
  if known_owner /usr/libexec/uke-pogo-desktop uke-pogo-desktop;then desktop=yes;fi
  [ "$auth" = yes ] || [ "$desktop" = yes ] || exit 0
  [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
  /usr/bin/mkdir -m700 "$state_dir"
  [ "$auth" != yes ] || snapshot system
  if [ "$desktop" = yes ];then
   for session_dir in /run/user/*;do
    [ -d "$session_dir" ] && [ ! -L "$session_dir" ] && [ -S "$session_dir/bus" ] || continue
    session_uid=${session_dir##*/};case "$session_uid" in ''|*[!0-9]*) continue;;esac
    snapshot "user-$session_uid"
   done
  fi
  (umask 077;printf '1\n' > "$state_dir/complete")
  ;;
 restore)
  [ -e "$state_dir" ] || [ -L "$state_dir" ] || exit 0
  private_directory || exit 1
  [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
   [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
  /usr/bin/systemctl daemon-reload || exit 1
  for entry in "$state_dir"/*;do
   [ "$entry" != "$state_dir/complete" ] || continue
   [ -f "$entry" ] && [ ! -L "$entry" ] && [ "$(/usr/bin/stat -c %%u "$entry")" = 0 ] &&
    [ "$(/usr/bin/stat -c %%a "$entry")" = 600 ] || exit 1
   target=${entry##*/}
   case "$target" in
    system) ;;
    user-*) session_uid=${target#user-};case "$session_uid" in ''|*[!0-9]*) exit 1;;esac
     # A session which logged out cannot be restarted by a package transaction.
     [ -S "/run/user/$session_uid/bus" ] || { /usr/bin/unlink "$entry";continue; }
     /usr/bin/systemctl --user --machine="$session_uid@.host" daemon-reload || exit 1;;
    *) exit 1;;
   esac
   [ "$(/usr/bin/wc -l < "$entry")" = 2 ] || exit 1
   enabled=$(/usr/bin/sed -n '1p' "$entry")
   active=$(/usr/bin/sed -n '2p' "$entry")
   valid_enabled "$enabled" && valid_active "$active" || exit 1
   current=$(control "$target" is-enabled 2>/dev/null) || :
   valid_enabled "$current" || exit 1
   case "$enabled:$current" in *masked*) /usr/bin/unlink "$entry";continue;;esac
   case "$enabled" in
    enabled) control "$target" enable || exit 1;;
    enabled-runtime) control "$target" enable --runtime || exit 1;;
   esac
   case "$active" in active|activating|reloading)
    running=$(control "$target" is-active 2>/dev/null) || :
    case "$running" in
     active|activating|reloading) control "$target" try-restart || exit 1;;
     inactive|failed) control "$target" start || exit 1;;
     deactivating) :;;
     *) exit 1;;
    esac
   ;;esac
   /usr/bin/unlink "$entry"
  done
  /usr/bin/unlink "$state_dir/complete"
  /usr/bin/rmdir "$state_dir"
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_POGO_SAVE_END
) || exit $?
(
# UKE_CONTROL_SSC_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-ssc-migration
units='uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-ssc-cache.service|uke-ssc-root.service|uke-ssc-sensors.service|iio-sensor-proxy.service|uke-adaptive-display.target) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-ssc-fileservice 2>/dev/null) || old=''
    if [ "$old" != uke-ssc-fileservice ];then
        old=$(/usr/bin/rpm -qf --qf '%%{NAME}' /usr/lib/systemd/system/uke-ssc-root.service 2>/dev/null) || exit 0
        [ "$old" = uke-control ] || exit 0
    fi
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^((uke-ssc-(cache|root|sensors)|iio-sensor-proxy)[.]service|uke-adaptive-display[.]target)$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=6 || n!=5)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_SSC_SAVE_END
) || exit $?
(
# UKE_CONTROL_ADAPTIVE_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-adaptive-migration
units='uke-adaptive-display.target'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-adaptive-display.target) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-adaptive-display 2>/dev/null) || exit 0
    [ "$old" = uke-adaptive-display ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-adaptive-display[.]target$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=2 || n!=1)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_ADAPTIVE_SAVE_END
) || exit $?
%sysusers_create_compat %{SOURCE54}
(
# UKE_CONTROL_BATTERY_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-battery-migration
units='uke-battery.service'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-battery.service) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-battery-observer 2>/dev/null) || exit 0
    [ "$old" = uke-battery-observer ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-battery[.]service$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=2 || n!=1)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_BATTERY_SAVE_END
) || exit $?
(
# UKE_CONTROL_CHARGE_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-charge-migration
units='uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-charge-control.service|uke-charge-automatic.service|uke-charge-protection.service) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-charge-control 2>/dev/null) || exit 0
    [ "$old" = uke-charge-control ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %%s\n%%s' "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-charge-(control|automatic|protection)[.]service$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=4 || n!=3)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_CHARGE_SAVE_END
) || exit $?
(
:
# UKE_CONTROL_SPEAKER_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans, with phase assigned by the recipe.
# No private calibration, audio command, unit trigger or background process.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-speaker-migration
template=uke-speaker-provision@.service
trap 'result=$?; if [ "$result" != 0 ]; then printf "%%s\n" "Uke speaker ownership migration refused; inspect the retained transaction record before retrying." >&2; fi' EXIT
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0

valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    printf '%%s\n' "$1" | /usr/bin/grep -Eq '^uke-speaker-provision@controlC[0-9]{1,3}[.]service$'
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%%s\n' "$value"
}
restore_enabled() {
    unit=$1; enabled=$2
    case "$enabled" in
        enabled) /usr/bin/systemctl --no-reload enable "$unit";;
        enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
        masked|masked-runtime)
            # Removal must not silently unmask a user's explicit preference.
            [ "$(enabled_state "$unit")" = "$enabled" ];;
        disabled|static|indirect) :;;
        *) return 1;;
    esac
}

case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%%{NAME}' uke-speaker-runtime 2>/dev/null) || exit 0
    [ "$old" = uke-speaker-runtime ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    template_state=$(enabled_state "$template") || exit 1
    # Read and validate all states before creating a transaction record.
    rows=$(/usr/bin/systemctl list-units --all --type=service --no-legend --plain 'uke-speaker-provision@*.service') || exit 1
    snapshot=''
    count=0
    for unit in $(printf '%%s\n' "$rows" | /usr/bin/awk 'NF {print $1}'); do
        valid_unit "$unit" || exit 1
        count=$((count+1)); [ "$count" -le 16 ] || exit 1
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ]; then
            case "$template_state:$enabled" in *masked*) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$(/usr/bin/mktemp "$state_dir/.units.XXXXXX") || exit 1
    (umask 077; printf 'V1 %%s %%s\n%%s' "$template_state" "$(/usr/bin/date +%%s)" "$snapshot" > "$temporary")
    /usr/bin/chmod 600 "$temporary"
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0; exit 1; }
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state_dir")" = 0:0:700 ] || exit 1
    record=$state_dir/units
    [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/stat -c %%s "$record")" -le 4096 ] || exit 1
    # Validate the complete record before any systemd mutation. No eval/source.
    /usr/bin/awk '
      NR==1 {if(NF!=3 || $1!="V1" || $2!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || $3!~/^[1-9][0-9]{0,15}$/)bad=1;next}
      {if(NF!=3 || $1!~/^uke-speaker-provision@controlC[0-9]{1,3}[.]service$/ || seen[$1]++ || $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || ($2=="active" && $3~/masked/))bad=1}
      END{if(NR<1 || NR>17)bad=1;exit bad}' "$record" || exit 1
    IFS=' ' read -r version template_state timestamp < "$record"
    now=$(/usr/bin/date +%%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    if [ "$template_state" = masked ] || [ "$template_state" = masked-runtime ]; then
        ! /usr/bin/grep -Eq '^[^ ]+ active ' "$record" || exit 1
    fi
    /usr/bin/systemctl daemon-reload
    restore_enabled "$template" "$template_state" || exit 1
    # The service's native --ensure-loaded takes the authenticated control lock
    # and returns without private-source reads/writes in healthy staged states.
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled; do
        restore_enabled "$unit" "$enabled" || exit 1
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
# UKE_CONTROL_SPEAKER_SAVE_END
) || exit $?
(
:
# UKE_CONTROL_TRANSFER_SAVE_BEGIN
if rpm -q uke-boot-integration >/dev/null 2>&1 || rpm -q uke-kernel-update >/dev/null 2>&1 || rpm -q uke-installation-profile >/dev/null 2>&1; then
 state=/run/uke-control-package-migration
 test ! -L "$state" && test ! -e "$state/units" || exit 1
 install -d -m700 "$state"
 : > "$state/units"
 chmod 600 "$state/units"
 for unit in uke-root-grow.service uke-rtc-sync.service uke-rtc-sync.timer uke-touchscreen.service uke-userdata-accept.service ; do
  enabled=$(systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(systemctl is-active "$unit" 2>/dev/null) || :
  case $enabled in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; not-found|'') continue;; *) exit 1;; esac
  case $active in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac
  printf '%%s\t%%s\t%%s\n' "$unit" "$enabled" "$active" >> "$state/units"
 done
fi
# UKE_CONTROL_TRANSFER_SAVE_END
) || exit $?
(
:
# Stop the previous root worker while its matching cleanup executable still exists.
/usr/bin/systemctl stop uke-control-flashlight.service >/dev/null 2>&1 || :

) || exit $?
(
:
(
old=uke-bluetooth; new=uke-boot-integration; unit=uke-bluetooth-address.service; phase=save
# POSIX shell fragment embedded only in RPM migration scriptlets.
# Restore the admitted state; never enable a previously disabled hardware path.
case "$phase" in
save)
    [ -d /run/systemd/system ] || exit 0
    test "$(rpm -q --qf '%%{NAME}' "$old" 2>/dev/null)" = "$old" || exit 0
    state=/run/uke-rpm-migration
    [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 1
    install -d -m700 "$state"
    enabled=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    active=$(systemctl is-active "$unit" 2>/dev/null) || :
    case "$enabled" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; *) exit 1;; esac
    case "$active" in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac
    (umask 077; printf '%%s\n%%s\n' "$enabled" "$active" >"$state/$new")
    ;;
restore)
    if [ "$new" = uke-iio-sensor-proxy ]; then
        if /usr/bin/selinuxenabled; then
            /usr/bin/semodule -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        else
            /usr/bin/semodule -n -s targeted -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        fi
    fi
    state=/run/uke-rpm-migration
    [ -f "$state/$new" ] && [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 0
    enabled=$(sed -n '1p' "$state/$new")
    active=$(sed -n '2p' "$state/$new")
    current=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    case "$current" in masked|masked-runtime) unlink "$state/$new"; exit 0;; esac
    systemctl daemon-reload || exit 1
    case "$enabled" in
        enabled) systemctl enable "$unit" || exit 1;;
        enabled-runtime) systemctl enable --runtime "$unit" || exit 1;;
        masked|masked-runtime) unlink "$state/$new"; exit 0;;
    esac
    if [ "$active" = active ]; then systemctl start "$unit" || exit 1; fi
    unlink "$state/$new"
    ;;
*) exit 1;;
esac
) || exit 1
(
old=uke-touchscreen; new=uke-boot-integration-touch; unit=uke-touchscreen.service; phase=save
# POSIX shell fragment embedded only in RPM migration scriptlets.
# Restore the admitted state; never enable a previously disabled hardware path.
case "$phase" in
save)
    [ -d /run/systemd/system ] || exit 0
    test "$(rpm -q --qf '%%{NAME}' "$old" 2>/dev/null)" = "$old" || exit 0
    state=/run/uke-rpm-migration
    [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 1
    install -d -m700 "$state"
    enabled=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    active=$(systemctl is-active "$unit" 2>/dev/null) || :
    case "$enabled" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; *) exit 1;; esac
    case "$active" in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac
    (umask 077; printf '%%s\n%%s\n' "$enabled" "$active" >"$state/$new")
    ;;
restore)
    if [ "$new" = uke-iio-sensor-proxy ]; then
        if /usr/bin/selinuxenabled; then
            /usr/bin/semodule -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        else
            /usr/bin/semodule -n -s targeted -i /usr/share/selinux/packages/uke-ssc.cil || exit 1
        fi
    fi
    state=/run/uke-rpm-migration
    [ -f "$state/$new" ] && [ ! -L "$state" ] && [ ! -L "$state/$new" ] || exit 0
    enabled=$(sed -n '1p' "$state/$new")
    active=$(sed -n '2p' "$state/$new")
    current=$(systemctl is-enabled "$unit" 2>/dev/null) || :
    case "$current" in masked|masked-runtime) unlink "$state/$new"; exit 0;; esac
    systemctl daemon-reload || exit 1
    case "$enabled" in
        enabled) systemctl enable "$unit" || exit 1;;
        enabled-runtime) systemctl enable --runtime "$unit" || exit 1;;
        masked|masked-runtime) unlink "$state/$new"; exit 0;;
    esac
    if [ "$active" = active ]; then systemctl start "$unit" || exit 1; fi
    unlink "$state/$new"
    ;;
*) exit 1;;
esac
) || exit 1

) || exit $?

(
# UKE_CONTROL_MOTION_SAVE_BEGIN
phase=save
# SPDX-License-Identifier: MIT
# Embedded package hooks. Only the owned, demand-activated Motion unit changes.
set -eu
export LC_ALL=C
state=/run/uke-control-motion-upgrade
unit=uke-control-motion.service
unit_file=/usr/lib/systemd/system/uke-control-motion.service
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;;*) return 1;;esac
}
owned_unit() {
 [ -f "$unit_file" ] && [ ! -L "$unit_file" ] || return 1
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$unit_file" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do [ "$owner" = uke-control ] || return 1;done <<OWNERS
$owners
OWNERS
}
read_record() {
 [ -d "$state" ] && [ ! -L "$state" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state")" = 0:0:700 ] || return 1
 [ -f "$state/unit" ] && [ ! -L "$state/unit" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$state/unit")" = 0:0:600:1 ] || return 1
 [ "$(/usr/bin/find "$state" -mindepth 1 -maxdepth 1 | /usr/bin/wc -l)" = 1 ] || return 1
 [ "$(/usr/bin/wc -c < "$state/unit")" -le 256 ] &&
 [ "$(/usr/bin/wc -l < "$state/unit")" = 3 ] || return 1
 IFS=' ' read -r version saved extra < "$state/unit"
 [ "$version" = 1 ] && [ -z "$extra" ] || return 1
 case "$saved" in ''|*[!0-9]*) return 1;;esac
 [ "${#saved}" -le 10 ] || return 1
 now=$(/usr/bin/date +%%s) || return 1
 [ "$saved" -le "$now" ] && [ "$((now-saved))" -le 3600 ] || return 1
 enabled=$(/usr/bin/sed -n '2p' "$state/unit")
 active=$(/usr/bin/sed -n '3p' "$state/unit")
 valid_enabled "$enabled" || return 1
 case "$active" in active|inactive|failed) :;;*) return 1;;esac
}
clear_record() { /usr/bin/unlink "$state/unit" || return 1; /usr/bin/rmdir "$state" || return 1; }
refresh_manager() {
 /usr/bin/systemctl daemon-reload || return 1
 /usr/bin/busctl --system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig || return 1
}
case "$phase" in
 save-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;exit 0;fi
  phase=save;;
 restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  # A newer provider restores after its complete transaction, not here.
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;exit 0;fi
  phase=restore;;
esac
case "$phase" in
 save)
  if [ -e "$state" ] || [ -L "$state" ];then exit 1;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then exit 0;fi
  owned_unit || exit 1
  enabled=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
  valid_enabled "$enabled" || exit 1
  case "$active" in active|inactive|failed) :;;*) exit 1;;esac
  /usr/bin/mkdir -m700 "$state" || exit 1
  captured=$(/usr/bin/date +%%s) || exit 1
  (umask 077;printf '1 %%s\n%%s\n%%s\n' "$captured" "$enabled" "$active" > "$state/unit") || exit 1
  read_record || exit 1
  # Stop while the outgoing executable and its recovery action still exist.
  if [ "$active" = active ];then /usr/bin/systemctl stop "$unit" || exit 1;fi
  ;;
 restore)
  has_record=no
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;has_record=yes;fi
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;fi
  # New installs also need the bus activation and policy configuration re-read.
  refresh_manager || exit 1
  if [ "$has_record" = no ];then exit 0;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then clear_record || exit 1;exit 0;fi
  current=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  valid_enabled "$current" || exit 1
  case "$enabled:$current" in *masked*) clear_record || exit 1;exit 0;;esac
  if [ "$active" = active ];then
   running=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
   case "$running" in
    active) :;;
    inactive|failed) /usr/bin/systemctl start "$unit" || exit 1;;
    *) exit 1;;
   esac
  fi
  clear_record || exit 1
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_MOTION_SAVE_END
) || exit $?

%preun -n uke-control
# UKE_USB_RECOVERY31_PREUN_BEGIN
%systemd_preun uke-usb-recovery.service
# UKE_USB_RECOVERY31_PREUN_END
# UKE_SETUP_CDC23_PREUN_BEGIN
%systemd_preun uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path
# UKE_SETUP_CDC23_PREUN_END
(
# UKE_CONTROL_MOTION_PREUN_SAVE_BEGIN
phase=save-upgrade
# SPDX-License-Identifier: MIT
# Embedded package hooks. Only the owned, demand-activated Motion unit changes.
set -eu
export LC_ALL=C
state=/run/uke-control-motion-upgrade
unit=uke-control-motion.service
unit_file=/usr/lib/systemd/system/uke-control-motion.service
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;;*) return 1;;esac
}
owned_unit() {
 [ -f "$unit_file" ] && [ ! -L "$unit_file" ] || return 1
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$unit_file" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do [ "$owner" = uke-control ] || return 1;done <<OWNERS
$owners
OWNERS
}
read_record() {
 [ -d "$state" ] && [ ! -L "$state" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a' "$state")" = 0:0:700 ] || return 1
 [ -f "$state/unit" ] && [ ! -L "$state/unit" ] &&
 [ "$(/usr/bin/stat -c '%%u:%%g:%%a:%%h' "$state/unit")" = 0:0:600:1 ] || return 1
 [ "$(/usr/bin/find "$state" -mindepth 1 -maxdepth 1 | /usr/bin/wc -l)" = 1 ] || return 1
 [ "$(/usr/bin/wc -c < "$state/unit")" -le 256 ] &&
 [ "$(/usr/bin/wc -l < "$state/unit")" = 3 ] || return 1
 IFS=' ' read -r version saved extra < "$state/unit"
 [ "$version" = 1 ] && [ -z "$extra" ] || return 1
 case "$saved" in ''|*[!0-9]*) return 1;;esac
 [ "${#saved}" -le 10 ] || return 1
 now=$(/usr/bin/date +%%s) || return 1
 [ "$saved" -le "$now" ] && [ "$((now-saved))" -le 3600 ] || return 1
 enabled=$(/usr/bin/sed -n '2p' "$state/unit")
 active=$(/usr/bin/sed -n '3p' "$state/unit")
 valid_enabled "$enabled" || return 1
 case "$active" in active|inactive|failed) :;;*) return 1;;esac
}
clear_record() { /usr/bin/unlink "$state/unit" || return 1; /usr/bin/rmdir "$state" || return 1; }
refresh_manager() {
 /usr/bin/systemctl daemon-reload || return 1
 /usr/bin/busctl --system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig || return 1
}
case "$phase" in
 save-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;exit 0;fi
  phase=save;;
 restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  # A newer provider restores after its complete transaction, not here.
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;exit 0;fi
  phase=restore;;
esac
case "$phase" in
 save)
  if [ -e "$state" ] || [ -L "$state" ];then exit 1;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then exit 0;fi
  owned_unit || exit 1
  enabled=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
  valid_enabled "$enabled" || exit 1
  case "$active" in active|inactive|failed) :;;*) exit 1;;esac
  /usr/bin/mkdir -m700 "$state" || exit 1
  captured=$(/usr/bin/date +%%s) || exit 1
  (umask 077;printf '1 %%s\n%%s\n%%s\n' "$captured" "$enabled" "$active" > "$state/unit") || exit 1
  read_record || exit 1
  # Stop while the outgoing executable and its recovery action still exist.
  if [ "$active" = active ];then /usr/bin/systemctl stop "$unit" || exit 1;fi
  ;;
 restore)
  has_record=no
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;has_record=yes;fi
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;fi
  # New installs also need the bus activation and policy configuration re-read.
  refresh_manager || exit 1
  if [ "$has_record" = no ];then exit 0;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then clear_record || exit 1;exit 0;fi
  current=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  valid_enabled "$current" || exit 1
  case "$enabled:$current" in *masked*) clear_record || exit 1;exit 0;;esac
  if [ "$active" = active ];then
   running=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
   case "$running" in
    active) :;;
    inactive|failed) /usr/bin/systemctl start "$unit" || exit 1;;
    *) exit 1;;
   esac
  fi
  clear_record || exit 1
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_MOTION_PREUN_SAVE_END
) || exit $?
(
# UKE_CONTROL_POGO_PREUN_SAVE_BEGIN
phase=save-upgrade
# SPDX-License-Identifier: MIT
# Embedded in Control RPM pre/posttrans. No hardware or TEE operation.
set -eu
export LC_ALL=C
state_dir=/run/uke-control-pogo-migration
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_active() {
 case "$1" in active|inactive|failed|activating|deactivating|reloading) return 0;; *) return 1;; esac
}
control() {
 scope=$1;shift
 case "$scope" in
  system) /usr/bin/systemctl "$@" uke-pogo-auth.service;;
  user-*) session_uid=${scope#user-};case "$session_uid" in ''|*[!0-9]*) return 1;;esac
   /usr/bin/systemctl --user --machine="$session_uid@.host" "$@" uke-pogo-desktop.service;;
  *) return 1;;
 esac
}
private_directory() {
 [ -d "$state_dir" ] && [ ! -L "$state_dir" ] &&
 [ "$(/usr/bin/stat -c %%u "$state_dir")" = 0 ] &&
 [ "$(/usr/bin/stat -c %%a "$state_dir")" = 700 ]
}
snapshot() {
 target=$1
 enabled=$(control "$target" is-enabled 2>/dev/null) || :
 active=$(control "$target" is-active 2>/dev/null) || :
 valid_enabled "$enabled" && valid_active "$active" || return 1
 (umask 077;printf '%%s\n%%s\n' "$enabled" "$active" > "$state_dir/$target")
}
known_owner() {
 owners=$(/usr/bin/rpm -qf --qf '%%{NAME}\n' "$1" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do
  case "$owner" in "$2"|uke-control) :;; *) return 1;;esac
 done <<OWNERS
$owners
OWNERS
}
case "$phase" in
 save-upgrade|restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  [ "$1" -gt 0 ] || exit 0
  if [ "$phase" = save-upgrade ];then
   if [ -e "$state_dir" ] || [ -L "$state_dir" ];then
    private_directory && [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
     [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
    # A newer provider's pre already captured the pre-transaction preference.
    exit 0
   fi
   phase=save
  else phase=restore;fi
 ;;
esac
case "$phase" in
 save)
  auth=no;desktop=no
  if known_owner /usr/libexec/uke-pogo-auth uke-pogo-auth;then auth=yes;fi
  if known_owner /usr/libexec/uke-pogo-desktop uke-pogo-desktop;then desktop=yes;fi
  [ "$auth" = yes ] || [ "$desktop" = yes ] || exit 0
  [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
  /usr/bin/mkdir -m700 "$state_dir"
  [ "$auth" != yes ] || snapshot system
  if [ "$desktop" = yes ];then
   for session_dir in /run/user/*;do
    [ -d "$session_dir" ] && [ ! -L "$session_dir" ] && [ -S "$session_dir/bus" ] || continue
    session_uid=${session_dir##*/};case "$session_uid" in ''|*[!0-9]*) continue;;esac
    snapshot "user-$session_uid"
   done
  fi
  (umask 077;printf '1\n' > "$state_dir/complete")
  ;;
 restore)
  [ -e "$state_dir" ] || [ -L "$state_dir" ] || exit 0
  private_directory || exit 1
  [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
   [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
  /usr/bin/systemctl daemon-reload || exit 1
  for entry in "$state_dir"/*;do
   [ "$entry" != "$state_dir/complete" ] || continue
   [ -f "$entry" ] && [ ! -L "$entry" ] && [ "$(/usr/bin/stat -c %%u "$entry")" = 0 ] &&
    [ "$(/usr/bin/stat -c %%a "$entry")" = 600 ] || exit 1
   target=${entry##*/}
   case "$target" in
    system) ;;
    user-*) session_uid=${target#user-};case "$session_uid" in ''|*[!0-9]*) exit 1;;esac
     # A session which logged out cannot be restarted by a package transaction.
     [ -S "/run/user/$session_uid/bus" ] || { /usr/bin/unlink "$entry";continue; }
     /usr/bin/systemctl --user --machine="$session_uid@.host" daemon-reload || exit 1;;
    *) exit 1;;
   esac
   [ "$(/usr/bin/wc -l < "$entry")" = 2 ] || exit 1
   enabled=$(/usr/bin/sed -n '1p' "$entry")
   active=$(/usr/bin/sed -n '2p' "$entry")
   valid_enabled "$enabled" && valid_active "$active" || exit 1
   current=$(control "$target" is-enabled 2>/dev/null) || :
   valid_enabled "$current" || exit 1
   case "$enabled:$current" in *masked*) /usr/bin/unlink "$entry";continue;;esac
   case "$enabled" in
    enabled) control "$target" enable || exit 1;;
    enabled-runtime) control "$target" enable --runtime || exit 1;;
   esac
   case "$active" in active|activating|reloading)
    running=$(control "$target" is-active 2>/dev/null) || :
    case "$running" in
     active|activating|reloading) control "$target" try-restart || exit 1;;
     inactive|failed) control "$target" start || exit 1;;
     deactivating) :;;
     *) exit 1;;
    esac
   ;;esac
   /usr/bin/unlink "$entry"
  done
  /usr/bin/unlink "$state_dir/complete"
  /usr/bin/rmdir "$state_dir"
  ;;
 *) exit 1;;
esac
# UKE_CONTROL_POGO_PREUN_SAVE_END
) || exit $?
%systemd_preun uke-pogo-auth.service
%systemd_user_preun uke-pogo-desktop.service
%systemd_preun uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service
%systemd_preun uke-adaptive-display.target
%systemd_preun uke-battery.service
%systemd_preun uke-charge-control.service uke-charge-automatic.service uke-charge-protection.service
%systemd_preun uke-speaker-provision@.service
(
:

 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-system-units uke-control-rgb.service || : 
fi 


 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-system-units uke-control.service uke-control-wake.service uke-control-cdc.service uke-control-flashlight.service || : 
fi 

if [ "$1" = 0 ]; then /usr/libexec/uke-control/torch-access --remove || exit 1; fi

 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-user-units uke-control-moisture-notify.service || : 
fi 

) || exit $?
(
:
if [ "$1" -eq 0 ] && [ -d /run/systemd/system ]; then
    systemctl stop uke-shutdown-trace.service || :
fi

 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-system-units uke-touchscreen.service || : 
fi 


) || exit $?
(
:

 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-system-units uke-userdata-accept.service || : 
fi 


) || exit $?
(
:

 
if [ $1 -eq 0 ] && [ -x "/usr/lib/systemd/systemd-update-helper" ]; then 
    # Package removal, not upgrade 
    /usr/lib/systemd/systemd-update-helper remove-system-units uke-root-grow.service uke-rtc-sync.service uke-rtc-sync.timer || : 
fi 

# Retire only our generated policy, including when downgrading to an older classifier.
if [ "$(readlink /etc/tmpfiles.d/journal-nocow.conf 2>/dev/null)" = '/usr/share/senemos/boot/uke/journal-nocow-disabled.conf' ]; then
    rm -f /etc/tmpfiles.d/journal-nocow.conf
fi

) || exit $?

%transfiletriggerin -n uke-control -- /usr/lib/senemos
(
:
if [ -d /run/systemd/system ] && [ -r /sys/firmware/devicetree/base/compatible ]; then
    if ! /usr/bin/uke-kernel-update auto; then
        echo 'Uke automatic boot update refused; inspect the reported cause before rebooting.' >&2
        exit 1
    fi
fi
) || exit $?

%transfiletriggerin -n uke-control -- /usr/lib/firmware/qcom/uke/sensors /usr/share/uke-sensors /usr/libexec/uke-ssc-cache
# UKE_CONTROL_SSC_REFRESH_BEGIN
if [ -d /run/systemd/system ] && [ -r /sys/firmware/devicetree/base/compatible ] &&
   grep -zFxq xiaomi,uke /sys/firmware/devicetree/base/compatible &&
   systemctl -q is-active uke-ssc-root.service;then
 adaptive_active=no
 if systemctl -q is-active uke-adaptive-display.target;then adaptive_active=yes;fi
 systemctl daemon-reload
 if systemctl try-restart uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service;then
  if [ "$adaptive_active" = yes ];then
   adaptive_mode=$(systemctl is-enabled uke-adaptive-display.target 2>/dev/null) || :
   case "$adaptive_mode" in masked|masked-runtime) :;;*) systemctl start uke-adaptive-display.target;;esac
  fi
 else
  echo 'Uke SSC cache refresh refused; inspect the service result before accepting the new sensor runtime.' >&2
 fi
fi
# UKE_CONTROL_SSC_REFRESH_END

# Capture before RPM replaces policy.kern; initial installation uses live policy.
%transfiletriggerun -n uke-control -P 2000000 -- /var/lib/selinux/targeted/active/modules/100 /var/lib/selinux/minimum/active/modules/100 /var/lib/selinux/mls/active/modules/100
if [ -x /usr/libexec/uke-control/selinux-kernel-abi ]; then /usr/libexec/uke-control/selinux-kernel-abi capture || exit $?; fi

# Stage before the original policy package runs its standard scriptlets.
%filetriggerin -n uke-control -P 2000000 -- /var/lib/selinux/targeted/active/modules/100 /var/lib/selinux/minimum/active/modules/100 /var/lib/selinux/mls/active/modules/100 /usr/libexec/uke-control/selinux-kernel-abi
if [ -x /usr/libexec/uke-control/selinux-kernel-abi ]; then /usr/libexec/uke-control/selinux-kernel-abi stage || exit $?; fi

# Rebuild after all policy producers and load only the selected live profile.
%transfiletriggerin -n uke-control -P 90000 -- /var/lib/selinux/targeted/active/modules/100 /var/lib/selinux/minimum/active/modules/100 /var/lib/selinux/mls/active/modules/100 /usr/share/selinux/packages /usr/libexec/uke-control/selinux-kernel-abi
if [ -x /usr/libexec/uke-control/selinux-kernel-abi ]; then /usr/libexec/uke-control/selinux-kernel-abi finish || exit $?; fi

%transfiletriggerin -n uke-control -P 2000000 -- /usr/share/senemos/uke /usr/lib/modules /usr/libexec/uke-cdsp-pogo-firmware
# Opportunistic transaction preparation. Trigger priority/failure is not a
# staging barrier: the native updater must prepare/verify its target itself.
# A chroot/unrelated machine must never inspect stock or create local firmware.
if [ -d /run/systemd/system ] && [ -r /sys/firmware/devicetree/base/compatible ] &&
   ! /usr/bin/systemd-detect-virt --quiet --chroot &&
   ! /usr/bin/systemd-detect-virt --quiet --container &&
   /usr/bin/tr '\000' '\n' < /sys/firmware/devicetree/base/compatible | /usr/bin/grep -Fxq xiaomi,uke; then
    %{_libexecdir}/uke-cdsp-pogo-firmware --sync-installed || exit 1
fi

%files
%{_datadir}/kwin/effects/uke-o82-resolution-continuity/
%{_libexecdir}/uke-control/display-dock
%{_userunitdir}/uke-control-display-dock.service
%{_userunitdir}/graphical-session.target.wants/uke-control-display-dock.service
%{_libexecdir}/uke-control/boot-notify
%{_userunitdir}/uke-boot-notify.service
%{_userunitdir}/uke-boot-notify.path
%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.service
%{_userunitdir}/graphical-session.target.wants/uke-boot-notify.path
%{_unitdir}/uke-userdata-fallback.service.d/50-uke-boot-notify.conf
%{_datadir}/polkit-1/actions/org.senemos.uke.kernel-notification.policy
%dir %attr(0755,root,root) %{_sharedstatedir}/senemos/kernel-notifications
/usr/libexec/uke-cdsp-pogo-firmware
%doc /usr/share/doc/uke-cdsp-pogo-firmware/README.md
# UKE_UKECTL29_FILES_BEGIN
/usr/libexec/uke-control/ukectl
/usr/lib/systemd/system/ukectl.service
# UKE_UKECTL29_FILES_END
# UKE_SWAP_READY28_FILES_BEGIN
%{_docdir}/uke-control/swap-ready28/
/usr/lib/systemd/system/uke-swap-ready.service
/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
/usr/lib/systemd/system/systemd-user-sessions.service.d/40-uke-swap-before-session.conf
# UKE_SWAP_READY28_FILES_END
# UKE_FACTORY_INPUTS27_FILES_BEGIN
%{_docdir}/uke-control/factory-inputs27/
# UKE_FACTORY_INPUTS27_FILES_END
# UKE_BOOT_STARTUP26_FILES_BEGIN
%{_docdir}/uke-control/startup26/
# UKE_BOOT_STARTUP26_FILES_END
# UKE_SUPER_SWAP25_FILES_BEGIN
/usr/libexec/uke-control/super-swap/
%{_docdir}/uke-control/LOGICAL-SWAP.md
%{_docdir}/uke-control/super-swap-results.txt
%{_docdir}/uke-control/setup-cdc-results.txt
# UKE_SUPER_SWAP25_FILES_END
# UKE_SUPER_BOOT24_FILES_BEGIN
%{_docdir}/uke-control/SUPER-ROOT-BOOT.md
%{_docdir}/uke-control/super-boot-native-results.txt
# UKE_SUPER_BOOT24_FILES_END
# UKE_SETUP_CDC23_FILES_BEGIN
/usr/lib/systemd/system/uke-control-setup-cdc.service
/usr/lib/systemd/system/uke-control-setup-console.service
/usr/lib/systemd/system/uke-control-setup-cdc-finish.service
/usr/lib/systemd/system/uke-control-setup-cdc-finish.path
/usr/lib/udev/rules.d/91-uke-control-setup-cdc.rules
/usr/lib/systemd/system-preset/81-uke-control-setup-cdc.preset
# UKE_SETUP_CDC23_FILES_END
/usr/libexec/uke-control/motion
/usr/libexec/uke-control/motion-policy
/usr/share/selinux/packages/uke_control_motion.cil
/usr/lib/systemd/system/uke-control-motion.service
/usr/share/dbus-1/system-services/org.senemos.Uke.Motion1.service
/usr/share/dbus-1/system.d/org.senemos.Uke.Motion1.conf
/usr/share/polkit-1/actions/org.senemos.uke.motion.policy
%doc %{_docdir}/uke-control/MOTION1.md
/usr/libexec/uke-control/selinux-kernel-abi
/usr/libexec/uke-control/compose-kernel-abi.awk
%doc %{_docdir}/uke-control/SELINUX-ORIGINAL.md
%doc %{_docdir}/uke-control/CLOSED-COVER-HEALTH.md
/usr/libexec/uke-pogo-auth
/usr/libexec/uke-pogo-desktop
/usr/lib/systemd/system/uke-pogo-auth.service
/usr/lib/systemd/system/uke-pogo-auth.service.d/pogo-attachment.conf
/usr/lib/systemd/user/uke-pogo-desktop.service
/usr/lib/udev/rules.d/70-uke-pogo.rules
/etc/xdg/autostart/uke-pogo-desktop.desktop
%license %{_licensedir}/%{name}/pogo
%doc %{_docdir}/%{name}/POGO-AUTH.md
%doc %{_docdir}/%{name}/POGO-DESKTOP.md
%doc %{_docdir}/%{name}/POGO-INTEGRATION.md
%doc %{_docdir}/%{name}/FASTRPC-PROVIDER.md
%dir /usr/share/uke-control
/usr/share/uke-control/fastrpc-root
/usr/lib/systemd/system/run-uke*.mount
/usr/libexec/uke-ssc-cache
/usr/libexec/uke-control/fastrpc-session
%dir /usr/lib64/uke-control
/usr/lib64/uke-control/libuke-fastrpc-notify.so
/usr/lib/systemd/system/uke-ssc-cache.service
/usr/lib/systemd/system/uke-ssc-root.service
/usr/lib/systemd/system/uke-ssc-sensors.service
/usr/lib/systemd/system/iio-sensor-proxy.service.d/uke-ssc.conf
/usr/lib/udev/rules.d/70-uke-ssc.rules
%dir /usr/share/licenses/uke-ssc-fileservice
%license /usr/share/licenses/uke-ssc-fileservice/COPYING
/usr/lib64/qt6/plugins/ksystemstats/libuke_adreno.so
/usr/share/plasma-systemmonitor/adreno.page
%config(noreplace) /etc/xdg/powerdevilrc
%config(noreplace) /etc/xdg/kwinoutputconfig.json
%config(noreplace) /etc/udev/rules.d/60-block-scheduler.rules
/usr/lib/systemd/system/uke-adaptive-display.target
/usr/lib/systemd/system-preset/80-uke-adaptive-display.preset
/usr/lib/udev/rules.d/71-uke-tablet-mode.rules
/usr/lib/udev/rules.d/61-uke-display-ddc.rules
/usr/libexec/senemos-uke/uke-ddc-config
/usr/lib/systemd/user/plasma-powerdevil.service.d/60-uke-display-ddc.conf
/usr/lib/systemd/sleep.conf.d/30-uke-s2idle.conf
/usr/lib/tmpfiles.d/uke-suspend.conf
%dir /usr/share/doc/uke-adaptive-display
%doc /usr/share/doc/uke-adaptive-display/*
%dir /usr/share/licenses/uke-adaptive-display
%license /usr/share/licenses/uke-adaptive-display/LICENSE
%doc ADAPTIVE-SERVICES.md
%{_bindir}/uke-battery
%{_libexecdir}/uke-offline-charge
%{_unitdir}/uke-battery.service
%{_unitdir}/uke-offline-charge.service
%{_unitdir}/uke-offline-charge.target
%{_presetdir}/90-uke-battery.preset
%{_sysusersdir}/uke-battery.conf
%{_datadir}/dbus-1/system.d/org.senemos.Uke.Battery1.conf
%{_datadir}/dbus-1/system-services/org.senemos.Uke.Battery1.service
%{_datadir}/uke-battery/battery-profile.json
%{_datadir}/uke-battery/model-profile.json
%dir %{_docdir}/uke-battery-observer
%doc %{_docdir}/uke-battery-observer/*
%dir %{_docdir}/uke-offline-charge
%doc %{_docdir}/uke-offline-charge/*
%dir %{_licensedir}/uke-battery-observer
%license %{_licensedir}/uke-battery-observer/LICENSE
%dir %{_licensedir}/uke-offline-charge
%license %{_licensedir}/uke-offline-charge/LICENSE
%doc BATTERY-SERVICES.md
%doc CHARGE-WAIT.md
%{_libexecdir}/uke-charge-control
%{_datadir}/uke-battery/protection-readiness.json
%{_unitdir}/uke-charge-control.service
%{_unitdir}/uke-charge-automatic.service
%{_unitdir}/uke-charge-protection.service
%{_udevrulesdir}/80-uke-charge-protection.rules
%{_presetdir}/91-uke-charge-control.preset
%config(noreplace) %{_sysconfdir}/uke-battery/control-profile.json
%config(noreplace) %{_sysconfdir}/UPower/UPower.conf.d/60-uke-battery-safety.conf
%dir %{_docdir}/uke-charge-control
%doc %{_docdir}/uke-charge-control/*
%dir %{_licensedir}/uke-charge-control
%license %{_licensedir}/uke-charge-control/LICENSE
%doc CHARGE-CONSOLIDATION.md
/usr/lib/udev/rules.d/89-uke-native-proximity.rules
%{_docdir}/uke-control/NATIVE-PROXIMITY-SELECTION.md
/usr/bin/uke-nfc-tag
/usr/lib/udev/rules.d/70-uke-nfc-ir.rules
%{_docdir}/uke-control/NFC-IR.md
%{_docdir}/uke-control/nfc-uapi-source.json
%{_docdir}/uke-control/NFC-CONSOLIDATION.md
%{_docdir}/uke-control/NFC-SOURCE.json
%license NFC-MIT-LICENSE NFC-GPL-2.0-only.txt NFC-Linux-syscall-note
/usr/bin/uke-speaker-runtime
/usr/bin/uke-speaker-gain-preflight
/usr/libexec/uke-speaker-prepare-data
/usr/lib/systemd/system/uke-speaker-provision@.service
/usr/lib/udev/rules.d/70-uke-speaker-provision.rules
%{_docdir}/uke-control/SPEAKER-RUNTIME-CONSOLIDATION.md
%license SPEAKER-RUNTIME-GPL-2.0 SPEAKER-RUNTIME-ORIGINAL-LICENSE
/usr/libexec/uke-control/brightness-curve
%config(noreplace) /etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh
%{_docdir}/uke-control/BRIGHTNESS-CURVE.md
%{_docdir}/uke-control/CRITICAL-ROLLBACK.md
%{_docdir}/uke-control/BOOT-HEALTH-OBSERVATION.md
/usr/libexec/uke-speaker-orientation
%config(noreplace) /etc/xdg/autostart/uke-speaker-orientation.desktop
%{_docdir}/uke-control/SPEAKER-ORIENTATION.md
%{_libexecdir}/uke-control/frontend-request
%{_libexecdir}/uke-control/update-status
%{_libexecdir}/uke-control/set-slot
%{_datadir}/polkit-1/actions/org.senemos.uke.update-status.policy
%license work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/LICENSE work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/shared/recovery/LICENSE-APACHE-2.0
%doc work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/README.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/CHANGELOG.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/CHARGING.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/DISPLAY.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/CHANNELS.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/SENSORS.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/DESKTOP.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/HARDWARE.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/DIAGNOSTIC.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/RESIZE.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/LIGHTS.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/CLI.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/BOOT-LAYOUT.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/OVERVIEW.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/REVIEW-2026-10-30.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/TUI.md work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/MICROPHONE.md
/usr/share/wireplumber/scripts/uke-microphone.lua
/usr/share/wireplumber/scripts/lib/uke-microphone-policy.lua
/usr/share/wireplumber/wireplumber.conf.d/96-uke-microphone-policy.conf
%{_unitdir}/plymouth-quit.service.d/50-uke-display-handoff.conf
%{_unitdir}/plymouth-quit-wait.service.d/50-uke-display-handoff.conf
/usr/bin/uke-control
/usr/bin/ukectl
/usr/bin/uke-control-tui
/usr/share/bash-completion/completions/uke-control
/usr/libexec/uke-control/rgb-led
/usr/libexec/uke-control/torch-access
/usr/lib/udev/rules.d/91-uke-control-torch.rules
/usr/share/dbus-1/system.d/org.senemos.Uke.Torch.conf
/usr/share/dbus-1/system-services/org.senemos.Uke.Torch.service
/usr/share/polkit-1/actions/org.senemos.uke.torch.policy
/usr/share/selinux/packages/uke_control_rgb.pp
/usr/lib/systemd/system/uke-control-rgb.service
/usr/lib/sysusers.d/uke-control-rgb.conf
/usr/lib/udev/rules.d/91-uke-control-rgb.rules
/usr/share/dbus-1/system.d/org.senemos.Uke.Rgb.conf
/usr/share/dbus-1/system-services/org.senemos.Uke.Rgb.service
/usr/share/polkit-1/actions/org.senemos.uke.rgb.policy
%dir /usr/libexec/uke-control
/usr/libexec/uke-control/packages
/usr/libexec/uke-control/resize-system
/usr/libexec/uke-control/layout-manager
/usr/lib/udev/rules.d/91-uke-control-layout.rules
/usr/libexec/uke-control/vrr-min-load
/usr/libexec/uke-control/hardware-policy
/usr/lib/systemd/system/uke-control-cpu-sleep.service
/usr/lib/systemd/system/systemd-suspend.service.d/90-uke-cpu-sleep.conf
/usr/share/polkit-1/actions/org.senemos.uke.hardware-policy.policy
/usr/lib/modprobe.d/60-uke-control-vrr.conf
/usr/lib64/libuke-control.so.0
/usr/lib/systemd/system/uke-control.service
/usr/lib/systemd/system/uke-control-wake.service
/usr/lib/systemd/system/uke-control-cdc.service
/usr/lib/systemd/system/uke-control-flashlight.service
/usr/lib/udev/rules.d/90-uke-control-cdc.rules
%dir %{_unitdir}/serial-getty@ttyGS0.service.d
%{_unitdir}/serial-getty@ttyGS0.service.d/40-uke-serial-reset.conf
%dir /usr/lib/systemd/system/sleep.target.requires
/usr/lib/systemd/system/sleep.target.requires/uke-control-wake.service
/usr/lib/systemd/system-preset/80-uke-control.preset
/usr/lib/systemd/user/uke-control-moisture-notify.service
/usr/lib/systemd/user-preset/80-uke-control-user.preset
%dir %attr(0700,root,root) /var/lib/uke-control

/usr/libexec/uke-control/tests
/usr/libexec/uke-control/ssc-discovery
/usr/libexec/uke-control/ssc-sample
/usr/share/applications/uke-control-tests.desktop


%license LICENSE
%{_libdir}/libuke-control-client.so.1

%license BOOT-RUNTIME-LICENSE
%license BOOT-RUNTIME-LICENSE.GPL-3.0
%license BOOT-RUNTIME-THP-LICENSE
%doc BOOT-RUNTIME-README.md
%doc BOOT-RUNTIME-BLUETOOTH.md
%doc BOOT-RUNTIME-USB-TYPEC.md
%doc BOOT-RUNTIME-NATIVE-POWER-PROFILES.md
%doc BOOT-RUNTIME-TOUCH-RUNTIME.md
%doc BOOT-RUNTIME-SHUTDOWN-TRACE.md
/usr/libexec/senemos-uke/uke-native-power-profile-ready
/usr/libexec/senemos-uke/uke-touchscreen
/usr/libexec/senemos-uke/uke-touch-thp
/usr/lib/systemd/system/uke-touchscreen.service
/usr/lib/systemd/system-preset/80-uke-touchscreen.preset
/usr/lib/udev/rules.d/70-uke-touchscreen.rules
/usr/lib/modules-load.d/uke-touchscreen.conf
/usr/lib/modprobe.d/uke-touchscreen.conf
%dir /usr/lib/firmware/novatek
%dir /usr/lib/firmware/novatek/uke
/usr/lib/firmware/novatek/uke/nt36532-normal.bin
/usr/lib/systemd/system/uke-native-power-profile-ready.service
/usr/lib/udev/rules.d/72-uke-native-power-profile.rules
/usr/libexec/senemos-uke/uke-wifi-factory
/usr/lib/systemd/system/uke-wifi-factory.service
/usr/libexec/uke-bluetooth-address
/usr/lib/systemd/system/uke-bluetooth-address.service
/usr/lib/udev/rules.d/70-uke-bluetooth.rules
/usr/bin/uke-boot-status
/usr/bin/uke-usb-port
/usr/lib/systemd/system/uke-usb-observer.service
/usr/share/dbus-1/system.d/org.senemos.Uke.USB1.conf
/usr/lib/systemd/system/chronyd.service.d/70-uke-clock.conf
/usr/lib/modules-load.d/uke-system-heap.conf
/usr/lib/udev/rules.d/72-uke-system-heap.rules
/usr/lib/clock-epoch
%dir %{_unitdir}/plasmalogin.service.d
%{_unitdir}/plasmalogin.service.d/30-uke-native-drm.conf
%{_libexecdir}/senemos-uke/uke-shutdown-trace
%{_unitdir}/uke-shutdown-trace.service
%dir %{_unitdir}/power-profiles-daemon.service.d
%{_unitdir}/power-profiles-daemon.service.d/30-uke-state-home.conf
/usr/lib/systemd/system-preset/80-uke-boot.preset
/usr/share/senemos/boot/uke
/usr/lib/systemd/system/uke-cdc-acm.service
/usr/libexec/senemos-uke/uke-usb-network
# UKE_USB_RECOVERY31_FILES_BEGIN
%{_unitdir}/uke-usb-recovery.service
%{_unitdir}/multi-user.target.wants/uke-usb-recovery.service
%{_udevrulesdir}/92-uke-usb-recovery.rules
/usr/lib/NetworkManager/conf.d/92-uke-usb-recovery.conf
/usr/lib/NetworkManager/system-connections/uke-usb-recovery.nmconnection
/usr/lib/firewalld/zones/uke-recovery.xml
%{_unitdir}/uke-root-capacity.service.d/20-journal.conf
# UKE_USB_RECOVERY31_FILES_END
%config(noreplace) /etc/senemos/usb-sharing
/usr/lib/systemd/system-preset/80-uke-usb.preset
/usr/lib/systemd/system/uke-usb-network.service
/usr/lib/systemd/system/uke-usb-network-shell.service
/usr/lib/modprobe.d/uke-coresight-bringup.conf
/usr/lib/sysctl.d/60-uke-security.conf

%doc BOOT-RUNTIME-KERNEL-UPDATES.md
%doc BOOT-RUNTIME-USERDATA-AB.md
%doc BOOT-RUNTIME-RTC-PERSISTENCE.md
%doc BOOT-RUNTIME-MANAGED-FIRMWARE.md
/usr/bin/uke-kernel-update
%dir /etc/senemos
%config(noreplace) /etc/senemos/rollback.conf
%config(noreplace) /etc/senemos/kernel-update.conf
%config(noreplace) /etc/senemos/userdata-update.conf
/usr/lib/systemd/system/uke-userdata-accept.service
/usr/lib/systemd/system/uke-userdata-fallback.service

/usr/lib/systemd/system/uke-rtc-counter-sync.service
/usr/lib/systemd/system/uke-rtc-counter-sync.timer
/usr/libexec/senemos-uke/uke-install-profile
/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
/usr/lib/systemd/system/uke-install-profile.service
/usr/lib/systemd/system/uke-root-grow.service
/usr/lib/systemd/system/uke-rtc-sync.service
/usr/lib/systemd/system/uke-rtc-sync.timer

%license O82-COLOR-LICENSE
%doc O82-COLOR-PROFILE.md
%{_bindir}/uke-o82-color-profile

/usr/share/dnf5/libdnf.conf.d/60-uke-filelists.conf
%doc KERNEL-CONSOLIDATION.md

%files -n uke-control-client-devel
%license LICENSE
%dir %{_includedir}/uke-control
%{_includedir}/uke-control/*.hpp
%{_libdir}/libuke-control-client.so
%{_libdir}/pkgconfig/uke-control-client.pc


%changelog
* Sat Nov 28 2026 Uke Linux Maintainers - 2.17.2
- Validate Fedora fuser results before removing owned ACM functions.

* Sat Nov 28 2026 Uke Linux Maintainers - 2.17.1
- Close the owned USB serial login prompt before releasing the ACM function.

* Sat Nov 28 2026 Uke contributors - 2.17.0-71
- Add optional sleep CPU hotplug and an admitted next-boot DSC sharing preference.

* Sat Nov 28 2026 Uke maintainers - 2.17.0-70
- Skip terminal reset sequences before ttyGS0 login.

* Sat Nov 28 2026 Uke Linux Maintainers - 2.17.0-68
- Preserve the fitted internal display content during resolution increases in Plasma.

* Sat Nov 28 2026 Uke Linux Maintainers - 2.17.0-67
- Verify the fitted provider before starting the protection observer.
- Bound automatic startup retries while the sysfs provider becomes ready.

* Sat Nov 28 2026 Uke Linux Maintainers - 2.17.0-66
- Start charge protection when its fitted auxiliary provider is ready.
- Tie service shutdown to provider removal or unbind.

* Fri Nov 27 2026 Uke Linux Maintainers - 2.17.0-65
- Wait for a valid KDE Wayland endpoint before reading display configuration.
- Keep the native boot notices and dock policy with the approved running-slot and firmware contracts.

* Fri Nov 27 2026 Uke Linux Maintainers - 2.17.0-62.10
- Show first-boot health notifications and retry the Plasma duplicate limit.
- Bind manual approval and on-demand fallback to the running trial and authenticated local user.
- Give the active desktop a bounded response window on ordinary enforced failures.

* Thu Nov 26 2026 Uke Linux Maintainers - 2.17.0-62.5
- Prepare native Iris firmware before activating an inactive kernel.

* Wed Nov 25 2026 Uke Linux Maintainers - 2.17.0-62.4
- Keep an older default boot bundle from replacing a healthy newer kernel.

* Mon Nov 23 2026 Uke Linux Maintainers - 2.17.0-62.1
- Grant the active local session access to MSM MST monitor DDC adapters.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-62
- Hand the native panel from Plymouth to the Plasma greeter with bounded recovery.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-61
- Skip replacing a trusted partition inventory when its contents match.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-60
- Admit source-versioned normal kernels and bind the ABI to the signed RPM owner.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-59
- Admit the exact normal 7.4.11 ABI and its merged kernel package ownership.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-58
- Use the fitted USB property readers when the paired inspection attribute is absent.
- Skip the module parameter probe for default VRR settings.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-57
- Recognize the reviewed normal 7.4.10 kernel in boot status, update staging and color profiles.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-56
- Update the native boot identity fixture for the verified normal releases.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-55
- Admit the next two normal kernel ABIs through the existing merged ownership checks.
- Read the swap signature and label together and retain all topology checks.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-53
- Decode logical swap metadata once and recheck the exact live metadata before and after mapper validation.

* Fri Nov 20 2026 Uke Linux Maintainers - 2.17.0-52
- Admit the next normal kernel ABI with existing merged RPM and recovery checks.
- Consolidate JSON geometry queries during logical swap validation.

* Thu Nov 19 2026 Uke Linux Maintainers - 2.17.0-49
- Scan block uevent records in Bash during logical swap admission.

* Thu Nov 19 2026 Uke Linux Maintainers - 2.17.0-48
- Start charge protection once both systemd power-supply devices are available.

* Thu Nov 19 2026 Uke Linux Maintainers - 2.17.0-47
- Start the existing charge protection service after both power supplies exist.
- Retain the auxiliary bind path for earlier kernels and the provider checks.

* Thu Nov 19 2026 Uke Linux Maintainers - 2.17.0-46
- Validate complete signed ownership layouts for the unstable kernel payload.
- Preserve the split-package path for both existing slots and other variants.

* Thu Nov 19 2026 Uke Linux Maintainers - 2.17.0-45
- Ship the verified CDSP and pogo firmware importer with its Control consumer.

* Thu Nov 19 2026 Uke contributors - 2.17.0-44
- Validate existing boot and RPM Image bytes in place to reduce transient allocations.

* Thu Nov 19 2026 Uke contributors - 2.17.0-43
- Install the terminal client in the runtime that already requires it.
- Run one full swap admission when the initramfs has prepared its logical device.

* Thu Nov 19 2026 Uke contributors - 2.17.0-42
- Report the full-to-design capacity ratio from coherent battery readings.

* Thu Nov 19 2026 Uke contributors - 2.17.0-41
- Let queued USB recovery reloads finish after an explicit opt-out.

* Wed Nov 18 2026 Uke contributors - 2.17.0-40
- Support an explicit DNF opt-out from USB Ethernet recovery.

* Wed Nov 18 2026 Uke contributors - 2.17.0-39
- Keep the exact-version dependencies aligned with the Control package release.
- Offer four firmware-managed charge bands and the ukectl command alias.

* Wed Nov 18 2026 Uke contributors - 2.17.0-36
- Run USB shutdown tracing on request only.

* Wed Nov 18 2026 Uke contributors - 2.17.0-35
- Activate the fixed USB recovery profile without waiting for its own carrier.

* Wed Nov 18 2026 Uke contributors - 2.17.0-34
- Reconcile the old NetworkManager decision for the bound recovery interface.

* Wed Nov 18 2026 Uke contributors - 2.17.0-33
- Manage only the enabled USB recovery Ethernet device.
- Send root capacity diagnostics to the journal during boot.

# UKE_USB_RECOVERY31_HISTORY_BEGIN
* Wed Nov 18 2026 Uke contributors - 2.17.0-32
- Activate the optional USB Ethernet link in the installation transaction.

# UKE_USB_RECOVERY31_HISTORY_END
# UKE_PIDFD30_HISTORY_BEGIN
* Wed Nov 18 2026 Uke contributors - 2.17.0-30
- Wait for child exit notifications in the bounded command runner.

# UKE_PIDFD30_HISTORY_END
# UKE_UKECTL29_HISTORY_BEGIN
* Wed Nov 18 2026 Uke contributors - 2.17.0-29
- Use a backend-only startup helper, exact mapper reuse and notification-driven LED waits.

# UKE_UKECTL29_HISTORY_END
# UKE_SWAP_READY28_HISTORY_BEGIN
* Tue Nov 17 2026 Uke contributors - 2.17.0-28
- Reuse current-boot full swap admission before user sessions; preserve changed-state fallback.

# UKE_SWAP_READY28_HISTORY_END
# UKE_FACTORY_INPUTS27_HISTORY_BEGIN
* Tue Nov 17 2026 Uke contributors - 2.17.0-27
- Prepare existing private panel and video inputs from exact admitted packaged OEM sources.

# UKE_FACTORY_INPUTS27_HISTORY_END
# UKE_BOOT_STARTUP26_HISTORY_BEGIN
* Mon Nov 16 2026 Uke contributors - 2.17.0-26
- Serialize boot swap and setup USB operations; prepare the clock floor before sandboxing.

# UKE_BOOT_STARTUP26_HISTORY_END
# UKE_SUPER_SWAP25_HISTORY_BEGIN
* Mon Nov 16 2026 Uke contributors - 2.17.0-25
- Admit the qualified logical swap before file fallback; use the installed systemctl for setup USB reboot.

# UKE_SUPER_SWAP25_HISTORY_END
# UKE_SUPER_BOOT24_HISTORY_BEGIN
* Mon Nov 16 2026 Uke contributors - 2.17.0-24
- Preserve capability-gated seed4 profiles and destination slot identity in the existing updater.

# UKE_SUPER_BOOT24_HISTORY_END
* Sat Nov 14 2026 Uke contributors - 2.17.0-19
- Add the demand-leased Motion1 interface within Control; keep production admission closed.
- Preserve original sensor policy, private ownership and package rollback behavior.
- Retain the complete Control16 source recipe and closed-cover health fix.

* Fri Nov 13 2026 Uke contributors - 2.17.0-16
- Recognize coherent native closed-cover display blanking during boot health.
- Preserve critical checks, continuous observation and rollback preferences.

* Thu Nov 12 2026 Uke contributors - 2.17.0-15
- Compose native kernel ABI definitions with the original Fedora SELinux policy.
- Preserve original policy sources, local module preferences and enforcing state.

* Thu Nov 12 2026 Uke contributors - 2.17.0-14
- Fold pogo authentication and Plasma integration into existing Control.
- Admit the qualified 7.3.3 bridge family and block on request or stop events.
- Preserve trusted input checks, original desktops and service preferences.

* Thu Nov 12 2026 Uke contributors - 2.17.0-12
- Use original Fedora FastRPC through the existing Control session integration.
- Disable implicit journal socket mounts in the installed SELinux namespace.
- Preserve the accepted sensor registry when moving to private noswap RAM.
- Retire the private cache mount only when the incoming package removes its provider.
- Retain source/native regression gates and remove the legacy protocol runtime.

* Thu Nov 12 2026 Uke contributors - 2.17.0-6
- Preserve active Adaptive targets across SSC ownership and cache refresh.
- Respect enable and mask preferences; retain all qualified compiled sources.

* Thu Nov 12 2026 Uke contributors - 2.17.0-5
- Fold SSC service ownership into Control while preserving the current provider.
- Qualify the original Fedora library session path without selecting it yet.

* Thu Nov 12 2026 Uke contributors - 2.17.0-4
- Retain Adaptive17 project plugin and policy in the existing Control package.
- Preserve configuration and target preferences across ownership migration.
- Keep original KDE dependencies conditional on their consumers.

* Wed Nov 11 2026 Uke contributors - 2.17.0-3
- Consolidate unchanged battery observation and offline UI into Control.
- Preserve battery activation preferences and the current charge helper.

* Wed Nov 11 2026 Uke contributors - 2.17.0-2
- Replace short periodic wait ticks with one interruptible deadline.
- Preserve the protection interval,helper ownership and persistent settings.

* Wed Nov 11 2026 Uke contributors - 2.17.0-1
- Fold the existing charge helper,units and config ownership into Control.
- Add the explicit fixed online cap bridge and retain all prior features.
- Preserve user unit/profile state and the original zero-argument client ABI.

* Wed Nov 11 2026 Uke contributors - 2.16.0-2
- Select native proximity input only on kernels declaring its versioned ABI.
- Retain raw IIO, legacy-kernel discovery and every existing Control feature.

* Tue Nov 10 2026 Uke contributors - 2.16.0-1
- Fold the native NFC tag CLI and exact NFC/IR session rules into Control.
- Preserve existing paths, kernel ABI, distribution IR utility and matching TUI.

* Mon Nov 09 2026 Uke contributors - 2.15.0-2
- Key acceptance backups by all observed GPT metadata, not entries alone.
- Merge pending brightness curves across unrelated shutdown settings.
- Preserve rollback, runtime ownership and original desktop packages.

* Mon Nov 09 2026 Uke contributors - 2.14.0-1
- Fold guarded speaker provisioning into Control with the existing paths and sound-device trigger.
- Restore active services without retransmitting already staged calibration; retain gain223.

* Mon Nov 09 2026 Uke contributors - 2.13.0-1
- Queue a reversible OSS normal brightness curve for a fresh original Plasma session.
- Preserve other output preferences and refuse active or changed configurations.

* Mon Nov 09 2026 Uke contributors - 2.12.3-1
- Detect recovered ADSP crashes through native remoteproc crash history.
- Log unavailable health once without granting critical fallback authority.

* Sun Nov 08 2026 Uke contributors - 2.11.0-1
- Observe unapproved boots in every rollback mode and record the failed health check.
- Keep off-mode approval, slot selection and reboot disabled.

* Sun Nov 08 2026 Uke contributors - 2.10.0-1
- Fold the event-driven speaker session bridge into Control.
- Retain the existing executable/autostart paths, settings and kernel compatibility.

* Sat Nov 07 2026 Uke contributors - 2.9.0-3
- Keep composed shell scriptlets valid after empty retained RPM macro expansions.
- Check every complete expanded lifecycle body before the native build.

* Sat Nov 07 2026 Uke contributors - 2.9.0-2
- Require the matching terminal package and retain the existing SDK separately.
- Fold native private-color provisioning into Control and admit canonical future unstable versions.

* Sat Nov 07 2026 Uke contributors - 2.9.0-1
- Consolidate retained control and kernel delivery components in one runtime RPM.
- Load filelists for reliable original DNF file-dependency resolution.

* Thu Nov 05 2026 Uke contributors - 2.8.1-26
- Report automatic staging and rollback policy in terminal-width columns.
* Thu Nov 05 2026 Uke contributors - 2.8.1-25
- Report automatic staging and rollback policy in terminal-width columns.
* Wed Nov 04 2026 Uke contributors - 2.8.1-11
- Preserve newer installed versions and unavailable extras during channel changes.
- Honor kernel version locks and skip boot work for unchanged selections.
- Keep exact synchronization explicit and add signed offline solver tests.
- Handle scriptlet pipe errors without truncating successful receipts.
