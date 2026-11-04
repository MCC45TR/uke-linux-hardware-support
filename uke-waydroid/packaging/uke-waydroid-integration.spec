Name:           uke-waydroid-integration
Version:        0.1.0
Release:        1%{?dist}
Summary:        Fedora Binderfs labels for Waydroid on Uke
License:        MIT
URL:            https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-waydroid
Source0:        %{name}-%{version}.tar.xz
BuildArch:      noarch
Requires:       waydroid waydroid-selinux selinux-policy-targeted
Requires(post): /usr/bin/semodule
Requires(postun): /usr/bin/semodule
Supplements:    (waydroid and uke-core-meta)

%description
Label Waydroid's three dynamically allocated anbox Binder devices with the
existing Fedora binder_device_t type. The original Waydroid SELinux module
then applies its existing device permissions. No application, kernel, service,
mount-wide context override or additional allow rule is provided.

%prep
%setup -q

%build
# The COPR source worker links this CIL against the complete Fedora policy
# using semodule -n. Binary packaging does not load policy in the worker.

%install
install -Dm0644 uke-waydroid/selinux/uke_waydroid_binder.cil \
    %{buildroot}%{_datadir}/selinux/packages/uke_waydroid_binder.cil

%check
cmp uke-waydroid/selinux/uke_waydroid_binder.cil \
    %{buildroot}%{_datadir}/selinux/packages/uke_waydroid_binder.cil

%post
%{_bindir}/semodule -X 400 -i %{_datadir}/selinux/packages/uke_waydroid_binder.cil

%postun
if [ "$1" -eq 0 ]; then
    %{_bindir}/semodule -X 400 -r uke_waydroid_binder
fi

%files
%doc uke-waydroid/README.md
%license uke-waydroid/LICENSE
%{_datadir}/selinux/packages/uke_waydroid_binder.cil

%changelog
* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Label the three dynamic Waydroid Binder device paths with Fedora's Binder type.
