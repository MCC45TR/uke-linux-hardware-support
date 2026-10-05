# Host-only transformation of exact-pinned official Fedora source specifications.
# This file is not executable and is never installed in a target RPM.
set -Eeuo pipefail
name=${1:?package}
top=${2:?RPM topdir}
project=${3:?repository root}
profile=$(jq -ce --arg name "$name" '.packages[] | select(.name==$name)' "$project/manifests/native-runtime.json")
version=$(jq -er .version <<< "$profile")
release=$(jq -er .release <<< "$profile")
native_release=$(jq -er .native_release <<< "$profile")
source_name=$(jq -er '.source_name // .name' <<< "$profile")
archive="$project/referances/fedora-srpms/$source_name-$version-$release.src.rpm"
mkdir -p "$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS} "$(dirname "$archive")"
if [[ ! -s $archive ]]; then
    curl -fL --retry 3 "https://kojipkgs.fedoraproject.org/packages/$source_name/$version/$release/src/$source_name-$version-$release.src.rpm" -o "$archive.part"
    mv "$archive.part" "$archive"
fi
[[ $(sha256sum "$archive" | cut -d ' ' -f1) == "$(jq -er .sha256 <<< "$profile")" ]]
(cd "$top/SOURCES"; rpm2cpio "$archive" | cpio -idmu --quiet)
if [[ $name == libstdcxx-uke-runtime ]]; then
    cp "$project/packaging/$name/$name.spec" "$top/SPECS/"
    cp "$project/src/native-runtime/runtime-smoke.cpp" "$top/SOURCES/"
    cp "$project/src/native-runtime/fedora-exported-symbols.txt" "$top/SOURCES/"
    cp "$project/packaging/$name/LICENSE.Boost" "$top/SOURCES/"
    cp "$archive" "$top/SOURCES/"
    rpmbuild -bs --nodeps --target aarch64 --define "_topdir $top" "$top/SPECS/$name.spec"
    exit 0
fi
spec="$top/SOURCES/$name.spec"
[[ -s $spec ]]
case $name in
    at-spi2-core) cleanup='rm -rf %{buildroot}%{_libdir}/python*';;
    libaccounts-glib) cleanup='rm -rf %{buildroot}%{_libdir}/python*';;
    gstreamer1) cleanup=$(cat <<'CLEAN'
rm -f %{buildroot}%{_libexecdir}/gstreamer-%{majorminor}/gst-hotdoc-plugins-scanner %{buildroot}%{_libexecdir}/gstreamer-%{majorminor}/gst-plugins-doc-cache-generator
rm -rf %{buildroot}%{_datadir}/gstreamer-%{majorminor}/gdb %{buildroot}%{_datadir}/gdb/auto-load
CLEAN
);;
    libwacom) cleanup='rm -f %{buildroot}%{_bindir}/libwacom-update-db %{buildroot}%{_bindir}/libwacom-show-stylus %{buildroot}%{_mandir}/man1/libwacom-show-stylus.1*';;
    plasma-workspace)
        cp "$project/src/native-runtime/calendar-migration.cpp" "$top/SOURCES/"
        cleanup=$(cat <<'CLEAN'
rm -f %{buildroot}%{_kf6_datadir}/kconf_update/migrate-calendar-to-plugin-id.py
install -Dm755 senemos-calendar-migration %{buildroot}%{_libdir}/kconf_update_bin/migrate-calendar-to-plugin-id
sed -i 's/Script=migrate-calendar-to-plugin-id.py/Script=migrate-calendar-to-plugin-id/' %{buildroot}%{_kf6_datadir}/kconf_update/migrate-calendar-to-plugin-id.upd
CLEAN
);;
    dolphin)
        cp "$project/src/native-runtime/dolphin-migration.cpp" "$top/SOURCES/"
        cleanup=$(cat <<'CLEAN'
for helper in dolphin_replace_view_mode_with_view_settings_in_toolbar dolphin_tab_key_shortcut_for_focus_other_view; do
    rm -f %{buildroot}%{_kf6_datadir}/kconf_update/$helper.py
    install -Dm755 senemos-dolphin-migration %{buildroot}%{_libdir}/kconf_update_bin/$helper
    sed -i "s/Script=$helper.py/Script=$helper/" %{buildroot}%{_kf6_datadir}/kconf_update/$helper.upd
done
find %{buildroot}%{_datadir}/doc/HTML -mindepth 2 -maxdepth 2 -type d -name dolphin -exec rm -rf {} +
CLEAN
);;
    *) exit 2;;
esac
# Section-aware edits preserve upstream source, native flags and file lists.
awk -v name="$name" -v release="$native_release" -v cleanup="$cleanup" '
    BEGIN { print "%global debug_package %{nil}" }
    function finish_install() {
        print "# Senemos Uke: keep Python outside every target payload."
        print cleanup
        print "if find %{buildroot} -type f \\( -name \"*.py\" -o -name \"*.pyc\" -o -name \"*.pyo\" -o -name \"python[0-9]*\" \\) -print | grep .; then exit 1; fi"
        print "while IFS= read -r -d \"\" file; do if head -c 256 \"$file\" | LC_ALL=C grep -aE \"^#!.*(python|pypy)\"; then exit 1; fi; done < <(find %{buildroot} -type f -print0)"
    }
    /^Release:/ { print "Release: " release "%{?dist}"; next }
    /^BuildRequires:.*%\{majmin_ver_kf6\}/ && name=="plasma-workspace" { sub(/%\{majmin_ver_kf6\}/,"6.7.91"); print; next }
    /^%install([[:space:]]|$)/ {
        if (name=="plasma-workspace") print "%{__cxx} -std=c++20 %{build_cxxflags} %{build_ldflags} $(pkg-config --cflags Qt6Core) %{SOURCE9999} -o senemos-calendar-migration $(pkg-config --libs Qt6Core)"
        if (name=="dolphin") print "%{__cxx} -std=c++20 %{build_cxxflags} %{build_ldflags} $(pkg-config --cflags Qt6Core Qt6Xml) %{SOURCE9999} -o senemos-dolphin-migration $(pkg-config --libs Qt6Core Qt6Xml)"
        install=1; print; next
    }
    install && /^%(check|files|pre|post|preun|postun|posttrans|pretrans|description|package|changelog|ldconfig_scriptlets)([[:space:]]|$)/ { finish_install(); install=0 }
    /^URL:/ && name=="plasma-workspace" { print; print "Source9999: calendar-migration.cpp"; next }
    /^URL:/ && name=="dolphin" { print; print "Source9999: dolphin-migration.cpp"; next }
    /^%package libs([[:space:]]|$)/ && name=="plasma-workspace" { print; print "Provides: senemos-native-runtime(plasma-workspace) = %{version}-%{release}"; next }
    /^License:/ && name!="plasma-workspace" {
        print; print "Provides: senemos-native-runtime(" name ") = %{version}-%{release}"
        print "ExclusiveArch: aarch64"
        print "BuildRequires: python3 = 3.15.0~rc2-1.fc46"
        if (name=="libwacom") print "BuildRequires: gcc"
        next
    }
    /^License:/ && name=="plasma-workspace" { print; print "ExclusiveArch: aarch64"; print "BuildRequires: python3 = 3.15.0~rc2-1.fc46"; next }
    /^BuildRequires:.*meson/ { print; print "BuildRequires: meson = 1.12.1-2.fc46"; next }
    /^BuildRequires:.*gobject-introspection-devel/ { print; print "BuildRequires: gobject-introspection-devel = 1.86.0-12.fc46"; next }
    name=="at-spi2-core" && /%\{python3_sitearch\}/ { next }
    name=="libaccounts-glib" && (/^Requires:.*python3-gobject/ || /%\{python3_sitearch\}/) { next }
    name=="gstreamer1" && /^%\{_libexecdir\}.*(gst-hotdoc-plugins-scanner|gst-plugins-doc-cache-generator)/ { next }
    name=="gstreamer1" && /^(%dir )?%\{_datadir\}.*(gstreamer.*\/gdb|gdb\/auto-load)/ { next }
    name=="libwacom" && /^Requires:.*python3-libevdev/ { next }
    name=="libwacom" && /^%\{_bindir\}.*(libwacom-update-db|libwacom-show-stylus)/ { next }
    name=="libwacom" && /^%\{_mandir\}.*libwacom-show-stylus/ { next }
    name=="plasma-workspace" && /^%\{_kf6_datadir\}.*migrate-calendar-to-plugin-id.py/ { print "%{_libdir}/kconf_update_bin/migrate-calendar-to-plugin-id"; next }
    name=="dolphin" && /^%\{_kf6_datadir\}.*dolphin_replace_view_mode_with_view_settings_in_toolbar.py/ { print "%{_libdir}/kconf_update_bin/dolphin_replace_view_mode_with_view_settings_in_toolbar"; next }
    name=="dolphin" && /^%\{_kf6_datadir\}.*dolphin_tab_key_shortcut_for_focus_other_view/ {
        print "%{_kf6_datadir}/kconf_update/dolphin_tab_key_shortcut_for_focus_other_view.upd"
        print "%{_libdir}/kconf_update_bin/dolphin_tab_key_shortcut_for_focus_other_view"
        next
    }
    name=="dolphin" && /^%find_lang/ { sub(/--with-html/, ""); print; next }
    /^%changelog/ {
        print
        print "* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - %{version}-" release
        print "- Rebuild the exact Fedora source with a Python-free native runtime payload."
        exit
    }
    { print }
    END { if (install) finish_install() }
' "$spec" > "$top/SPECS/$name.spec"
# The original complete source RPM is retained below referances; only the
# inspected generated specification controls this development candidate.
mv "$spec" "$top/SPECS/$name.fedora-original.spec"
[[ $(grep -c '^%install' "$top/SPECS/$name.spec") == 1 ]]
grep -F 'Senemos Uke: keep Python outside every target payload.' "$top/SPECS/$name.spec" >/dev/null
rpmbuild -bs --nodeps --target aarch64 --define "_topdir $top" "$top/SPECS/$name.spec"
