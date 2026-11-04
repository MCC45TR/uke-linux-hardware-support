Name:           uke-waydroid-integration
Version:        0.1.0
Release:        6%{?dist}
Summary:        Fedora Binderfs labels for Waydroid on Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-waydroid
Source0:        %{name}-%{version}.tar.xz
BuildArch:      noarch
Requires:       waydroid waydroid-selinux selinux-policy-targeted
Requires(post): /usr/bin/semodule
Requires(post): /usr/bin/systemd-tmpfiles
Requires:       coreutils gawk
Requires:       uke-waydroid-diagnostics >= 0.1.0-2
Requires(postun): /usr/bin/semodule
Supplements:    (waydroid and uke-core-meta)

%description
Label Waydroid's three dynamically allocated anbox Binder devices with the
existing Fedora binder_device_t type. The original Waydroid SELinux module
then applies its existing device permissions. A directory-read permission and
three tmpfiles aliases support allocation and startup after reboot. The
packaged desktop preference enables separate Wayland application windows.
The original application and kernel remain distribution components.

%prep
%setup -q

%build
# The COPR source worker links this CIL against the complete Fedora policy
# using semodule -n. Binary packaging does not load policy in the worker.

%install
install -Dm0644 uke-waydroid/selinux/uke_waydroid_binder.cil \
    %{buildroot}%{_datadir}/selinux/packages/uke_waydroid_binder.cil
install -Dm0644 uke-waydroid/config/uke-waydroid.conf \
    %{buildroot}%{_prefix}/lib/tmpfiles.d/uke-waydroid.conf
install -Dm0644 uke-waydroid/config/50-uke-waydroid.conf \
    %{buildroot}%{_prefix}/lib/systemd/system/waydroid-container.service.d/50-uke-waydroid.conf
install -Dm0644 uke-waydroid/config/uke-defaults.conf \
    %{buildroot}%{_sysconfdir}/waydroid/uke-defaults.conf
install -Dm0755 uke-waydroid/runtime/uke-waydroid-prepare \
    %{buildroot}%{_libexecdir}/uke-waydroid-prepare

%check
cmp uke-waydroid/selinux/uke_waydroid_binder.cil \
    %{buildroot}%{_datadir}/selinux/packages/uke_waydroid_binder.cil
cmp uke-waydroid/config/uke-waydroid.conf \
    %{buildroot}%{_prefix}/lib/tmpfiles.d/uke-waydroid.conf
cmp uke-waydroid/config/50-uke-waydroid.conf \
    %{buildroot}%{_prefix}/lib/systemd/system/waydroid-container.service.d/50-uke-waydroid.conf
cmp uke-waydroid/runtime/uke-waydroid-prepare \
    %{buildroot}%{_libexecdir}/uke-waydroid-prepare
sh uke-waydroid/tests/prepare.sh

%post
%{_bindir}/semodule -X 400 -i %{_datadir}/selinux/packages/uke_waydroid_binder.cil || exit 1
%{_bindir}/systemd-tmpfiles --create %{_prefix}/lib/tmpfiles.d/uke-waydroid.conf || exit 1
%{_libexecdir}/uke-waydroid-prepare || exit 1
%{_bindir}/systemctl daemon-reload

%postun
if [ "$1" -eq 0 ]; then
    %{_bindir}/semodule -X 400 -r uke_waydroid_binder || exit 1
fi
%{_bindir}/systemctl daemon-reload

%files
%doc uke-waydroid/README.md
%license uke-waydroid/LICENSE
%{_datadir}/selinux/packages/uke_waydroid_binder.cil
%{_prefix}/lib/tmpfiles.d/uke-waydroid.conf
%{_prefix}/lib/systemd/system/waydroid-container.service.d/50-uke-waydroid.conf
%config(noreplace) %{_sysconfdir}/waydroid/uke-defaults.conf
%{_libexecdir}/uke-waydroid-prepare

%changelog
* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Deliver the Android Vulkan qualification tool with a normal DNF update.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Align the Uke Android Mesa UBWC hint with the host and select native Turnip.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Use Fedora's normalized interpreter path and order startup after Binderfs.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Persist desktop window integration through the original Waydroid properties.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Permit Binderfs enumeration and create the three aliases before daemon startup.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Label the three dynamic Waydroid Binder device paths with Fedora's Binder type.
