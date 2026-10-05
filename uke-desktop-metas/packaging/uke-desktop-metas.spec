%global debug_package %{nil}
Name: uke-desktop-metas
Version: 1.0.0
Release: 3%{?dist}
Summary: Explicit desktop package selections for the Uke development platform
License: MIT
URL: https://github.com/MCC45TR/uke-desktop-metas
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz
Requires: uke-core-meta >= 1.0.0-3
Obsoletes: kde-plasma-uke-meta < 1.0.0-3
%description
Desktop selection metadata for Uke. The base installs no graphical session.
Display, touch, GPU and tablet boot remain independently unqualified.
%prep
%setup -q
%build
%install
install -Dm644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/desktops/profile.json
%files
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/profile.json
%changelog
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Withdraw the derivative-dependent KDE selection and retain upstream desktop policy metadata.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Require audited native runtime variants for Python-free Plasma dependencies.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
