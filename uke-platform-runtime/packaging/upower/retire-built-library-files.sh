#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR buildroot only; the original Fedora packages own these paths at runtime.
set -Eeuo pipefail
[[ $# == 1 && ! -e /etc/senemos/kernel-update.conf ]]
root=$(realpath "$1")
case "$root" in /builddir/*/BUILDROOT|/builddir/*/BUILDROOT/uke-upower-*) :;;*) exit 1;;esac
[[ -f $root/usr/libexec/upowerd ]]
rm -- "$root"/usr/lib64/libupower-glib.so "$root"/usr/lib64/libupower-glib.so.3 "$root"/usr/lib64/libupower-glib.so.3.1.2
rm -- "$root"/usr/lib64/girepository-1.0/UPowerGlib-1.0.typelib "$root"/usr/lib64/pkgconfig/upower-glib.pc "$root"/usr/share/gir-1.0/UPowerGlib-1.0.gir
rm -- "$root"/usr/share/dbus-1/interfaces/org.freedesktop.UPower*.xml
find "$root/usr/include/libupower-glib" "$root/usr/share/gtk-doc/html/UPower" -depth -delete
rmdir --ignore-fail-on-non-empty "$root/usr/share/gtk-doc/html" "$root/usr/share/gtk-doc"
