%global debug_package %{nil}
%global _build_id_links none
%global __brp_strip %{nil}
%global __brp_strip_comment_note %{nil}

Name:           orca-slicer
Version:        2.4.2
Release:        1%{?dist}
Summary:        3D printing slicer for FFF printers
License:        AGPL-3.0-only
URL:            https://github.com/OrcaSlicer/OrcaSlicer
Source0:        %{url}/releases/download/v%{version}/OrcaSlicer_Linux_AppImage_Ubuntu2404_aarch64_V%{version}.AppImage
Source1:        https://raw.githubusercontent.com/OrcaSlicer/OrcaSlicer/8500fcdccaa10b5099ac20d252af3a7c560046f1/LICENSE.txt
ExclusiveArch:  aarch64
BuildRequires:  binutils
BuildRequires:  chrpath
BuildRequires:  desktop-file-utils
BuildRequires:  squashfs-tools
Requires:       coreutils
Requires:       findutils
Requires:       grep
Requires:       sed

%description
OrcaSlicer prepares 3D models for fused filament fabrication. This RPM uses
the official OrcaSlicer ARM64 release with Fedora's shared libraries.
Corresponding source is available from the OrcaSlicer v%{version} release.

%prep
%setup -q -c -T
printf '%%s  %%s\n' e1a07275a25f176626c55a5df39e91bc4476d8c28ee4a3192ff758e29dd5c3ba '%{SOURCE0}' | sha256sum -c --strict
printf '%%s  %%s\n' 57c8ff33c9c0cfc3ef00e650a1cc910d7ee479a8bc509f6c9209a7c2a11399d6 '%{SOURCE1}' | sha256sum -c --strict
# Extract the pinned Type 2 AppImage without executing its runtime.
unsquashfs -processors 2 -no-progress -offset 936456 -d app '%{SOURCE0}' \
    AppRun bin/orca-slicer libexec/orca-slicer-env resources \
    OrcaSlicer.png com.orcaslicer.OrcaSlicer.desktop
LC_ALL=C readelf -h app/bin/orca-slicer | grep -Eq 'Machine: *AArch64'
test -z "$(LC_ALL=C readelf -d app/bin/orca-slicer | grep -i libpython)"
# The profile checker is a source maintenance helper.
rm -- app/resources/profiles/check_unused_setting_id.py
test -z "$(find app -iname '*.py' -o -iname '*.pyc' -o -iname '*libpython*')"
# The upstream CI build path ends with an empty loader search entry.
chrpath --delete app/bin/orca-slicer
install -m0644 '%{SOURCE1}' LICENSE.txt

%build

%install
appdir='%{buildroot}%{_libdir}/orcaslicer'
install -dm0755 "$appdir" '%{buildroot}%{_bindir}'
cp -a app/AppRun app/bin app/libexec app/resources "$appdir/"
ln -s '%{_libdir}/orcaslicer/AppRun' '%{buildroot}%{_bindir}/orca-slicer'
install -Dm0644 app/OrcaSlicer.png \
    '%{buildroot}%{_datadir}/icons/hicolor/192x192/apps/OrcaSlicer.png'
desktop-file-install --dir='%{buildroot}%{_datadir}/applications' \
    --set-key=Exec --set-value='orca-slicer %%F' app/com.orcaslicer.OrcaSlicer.desktop

%files
%license LICENSE.txt
%{_bindir}/orca-slicer
%{_libdir}/orcaslicer/
%{_datadir}/applications/com.orcaslicer.OrcaSlicer.desktop
%{_datadir}/icons/hicolor/192x192/apps/OrcaSlicer.png

%changelog
* Sat Nov 21 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 2.4.2-1
- Package the official ARM64 AppImage for Fedora Rawhide.
