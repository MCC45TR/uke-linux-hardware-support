%global debug_package %{nil}
Name: uke-plymonth
Version: 1.1.0
Release: 1%{?dist}
Summary: Senemos Uke boot and shutdown Plymouth theme
License: MIT AND GPL-2.0-or-later
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-packaging
Source0: %{name}-%{version}.tar.xz
BuildRequires: tar xz
BuildRequires: systemd-rpm-macros
BuildRequires: gcc-c++ clang compiler-rt libasan libubsan pkgconfig(ply-splash-graphics) >= 26.134.222 pkgconfig(libdrm)
BuildRequires: plymouth-plugin-label abattis-cantarell-fonts
Requires: plymouth plymouth-plugin-label
Requires: abattis-cantarell-fonts
Requires(post): plymouth-scripts
Requires(preun): plymouth-scripts
Provides: plymouth-uke = 1.0.2-1
Obsoletes: plymouth-uke < 1.0.2-1
Conflicts: plymouth-uke
%description
Xiaomi and Fedora artwork with boot/update progress and a pulsing shutdown bar.
The original Fedora Plymouth engine loads this theme's native splash plugin.
DRM/EDID physical dimensions keep artwork centimetres constant across displays.
Boot, shutdown and Fedora offline updates share the theme. Installation selects
the theme when a distribution default is in use; custom themes remain selected.
No initramfs, boot image, partition or boot-slot selection is changed.
%prep
%setup -q
%build
c++ %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror -fPIC -shared \
    src/plymouth-uke/plugin.cpp $(pkg-config --cflags --libs ply-splash-graphics libdrm) \
    %{build_ldflags} -Wl,-z,defs -o uke-physical.so
%install
install -d %{buildroot}%{_datadir}/plymouth/themes/senemos-uke
install -m644 src/plymouth-uke/*.png src/plymouth-uke/*.plymouth %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/
install -Dm755 uke-physical.so %{buildroot}%{_libdir}/plymouth/uke-physical.so
for item in start:boot poweroff:shutdown reboot:reboot halt:shutdown; do
    unit=${item%%:*}
    mode=${item#*:}
    directory=%{buildroot}%{_unitdir}/plymouth-$unit.service.d
    install -d "$directory"
    cat >"$directory/50-uke-theme.conf" <<EOF
[Service]
ExecStart=
ExecStart=/usr/bin/plymouthd --mode=$mode --pid-file=/run/plymouth/pid --attach-to-session --graphical-boot
EOF
done
install -d %{buildroot}%{_unitdir}/system-update.target.d
cat > %{buildroot}%{_unitdir}/system-update.target.d/50-uke-plymouth.conf <<'EOF'
[Unit]
Wants=plymouth-start.service
After=plymouth-start.service
EOF
%check
ulimit -c 0
for compiler in g++ clang++; do
 for mode in plain sanitized; do
  flags=''
  if [ "$mode" = sanitized ]; then flags='-fsanitize=address,undefined -fno-omit-frame-pointer'; fi
  "$compiler" -std=c++17 -O1 -g -UNDEBUG -Wall -Wextra -Werror $flags \
    tests/plymouth-physical.cpp $(pkg-config --cflags --libs ply-splash-graphics libdrm) -o physical-test
  ASAN_OPTIONS=detect_leaks=0 timeout --kill-after=2s 30s ./physical-test "$PWD/src/plymouth-uke" "$PWD/theme-test.conf"
 done
done
test -s %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/senemos-uke.plymouth
test -s %{buildroot}%{_libdir}/plymouth/uke-physical.so
! readelf -d uke-physical.so | grep -q TEXTREL
nm -D uke-physical.so | grep ' T ply_boot_splash_plugin_get_interface$'
%post
case "$(plymouth-set-default-theme)" in
    bgrt|spinner|charge|text|details|tribar|senemos-uke|'')
        plymouth-set-default-theme senemos-uke ;;
esac
%preun
if [ "$1" -eq 0 ] && [ "$(plymouth-set-default-theme)" = senemos-uke ]; then
    plymouth-set-default-theme --reset
fi
%files
%license LICENSE
%license src/plymouth-uke/LICENSE.Nabu
%doc README.md
%{_datadir}/plymouth/themes/senemos-uke/
%{_libdir}/plymouth/uke-physical.so
%{_unitdir}/plymouth-start.service.d/
%{_unitdir}/plymouth-poweroff.service.d/
%{_unitdir}/plymouth-reboot.service.d/
%{_unitdir}/plymouth-halt.service.d/
%{_unitdir}/system-update.target.d/50-uke-plymouth.conf

%changelog
* Tue Nov 17 2026 Uke contributors - 1.1.0-1
- Keep artwork physical size using cached per-display DRM dimensions and original Plymouth APIs.
- Preserve password prompts and offline update progress; redraw only the shutdown bar at 8 Hz.

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-4
- Require the original text renderer used by the theme and script console viewer.
- Check actual label rendering as well as script syntax in the package build.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-3
- Adapt the attributed Nabu artwork for removable Uke boot and shutdown visuals.
- Use original Fedora services without rebuilding or writing a boot image.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-1
- Rename the optional theme to uke-plymonth with a versioned upgrade path.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
