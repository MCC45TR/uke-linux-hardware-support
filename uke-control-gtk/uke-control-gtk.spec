Name: uke-control-gtk
Version: 1.0.0
Release: 1%{?dist}
Summary: Native GTK frontend for Uke device controls
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
BuildRequires: gcc-c++ cmake
BuildRequires: pkgconfig(gtk4) >= 4.12
BuildRequires: pkgconfig(libadwaita-1) >= 1.5
BuildRequires: pkgconfig(jsoncpp)
BuildRequires: pkgconfig(uke-control-client) >= 2.8.0
BuildRequires: desktop-file-utils
Requires: uke-control >= 2.8.0
Requires: gnome-terminal

%description
Native C++ GTK 4 and libadwaita application for Uke observations and guarded
control requests. Includes task navigation, typed forms, asynchronous reads,
explicit request review and simulation. Changes continue in a distribution
terminal through the original Uke Control backend.

%prep
%autosetup

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.Control.Gtk.desktop

%check
%ctest

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-gtk
%{_datadir}/applications/org.uke.Control.Gtk.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.Control.Gtk.svg
