# Host-only transformation of the exact-pinned original Fedora specification.
/^Release:/ { print "Release: " release "%{?dist}"; releases++; next }
/^License:/ { print; print "ExclusiveArch: aarch64"; next }
/^## upstreamable patches$/ {
    print
    print "Source9995: qt6-qtvirtualkeyboard-6.12.0-1.fc46.src.rpm"
    print "Source9998: qt-virtualkeyboard-sound.cpp"
    print "Source9999: qt-virtualkeyboard-build-source.json"
    sources++; next
}
/^BuildRequires: qt6-(qtbase-devel|qtdeclarative-devel|qtsvg-devel) >=/ {
    print "BuildRequires: " $2 " = %{version}"; qtdeps++; next
}
/^BuildRequires: qt6-qtbase-private-devel$/ { print "BuildRequires: qt6-qtbase-private-devel = %{version}"; next }
/^BuildRequires: hunspell-devel$/ {
    print; print "BuildRequires: qt6-qtmultimedia-devel = %{version}\nBuildRequires: pkgconf-pkg-config"; multimedia++; next
}
/^%cmake_qt6/ {
    print "cp %{SOURCE9999} qt-virtualkeyboard-build-source.json"
    print; print "  -DFEATURE_vkb_sound_effects:BOOL=ON \\"; feature++; next
}
/^%files$/ {
    print "%check"
    print "test -s %{buildroot}%{_qt6_qmldir}/QtQuick/VirtualKeyboard/Components/MultiSoundEffect.qml"
    print "grep -Eq '^MultiSoundEffect [0-9].*MultiSoundEffect[.]qml$' %{buildroot}%{_qt6_qmldir}/QtQuick/VirtualKeyboard/Components/qmldir"
    print "%{__cxx} -std=c++17 -O2 -Wall -Wextra -Werror %{SOURCE9998} -o qt-sound-contract $(pkg-config --cflags --libs Qt6Gui Qt6Qml Qt6Quick)"
    print "mkdir -m700 qt-sound-runtime"
    print "QT_QPA_PLATFORM=offscreen XDG_RUNTIME_DIR=\"$PWD/qt-sound-runtime\" QML_IMPORT_PATH=%{buildroot}%{_qt6_qmldir} LD_LIBRARY_PATH=%{buildroot}%{_qt6_libdir} timeout 30 ./qt-sound-contract %{buildroot}%{_qt6_qmldir}/QtQuick/VirtualKeyboard/Components"
    print ""
    print; print "%doc qt-virtualkeyboard-build-source.json"; checks++; next
}
/^%changelog/ {
    print
    print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 6.12.0-" release
    print "- Enable the upstream sound component used by the original keyboard."
    print "- Check actual packaged QML creation offscreen without audio playback."
    print ""
    history=1; next
}
{ print }
END { if(releases!=1 || sources!=1 || qtdeps!=3 || multimedia!=1 || feature!=1 || checks!=1 || !history)exit 1 }
