#!/usr/bin/bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
[[ $# == 2 && ! -e /etc/senemos/kernel-update.conf ]]
root=$(realpath "$1");built=$(realpath "$2")
case "$root" in /builddir/*/BUILDROOT|/builddir/*/BUILDROOT/uke-upower-*) :;;*) exit 1;;esac
[[ -f $root/usr/libexec/upowerd && -f $built ]]
[[ $(rpm -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' upower-libs) == upower-libs-1.91.5-1.fc46.aarch64 ]]
[[ $(rpm -qf --qf '%{NAME}' /usr/lib64/libupower-glib.so.3.1.2) == upower-libs ]]
[[ ! -e $root/usr/lib64/libupower-glib.so.3 && ! -e $root/usr/lib64/girepository-1.0/UPowerGlib-1.0.typelib && ! -d $root/usr/include/libupower-glib && ! -d $root/usr/share/gtk-doc/html/UPower ]]
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
symbols(){ readelf --dyn-syms --wide "$1" | awk '$1~/^[0-9]+:$/ && $7!="UND" && ($5=="GLOBAL"||$5=="WEAK"){print $4,$5,$6,$8,($4=="OBJECT"?$3:"code")}' | sort; }
symbols "$built" > "$work/built"
symbols /usr/lib64/libupower-glib.so.3.1.2 > "$work/fedora"
cmp "$work/built" "$work/fedora"
for binary in usr/libexec/upowerd usr/bin/upower;do
 LD_LIBRARY_PATH=/usr/lib64 /lib/ld-linux-aarch64.so.1 --list "$root/$binary" > "$work/loader"
 grep -Fq 'libupower-glib.so.3 => /usr/lib64/libupower-glib.so.3 ' "$work/loader"
 ! grep -Eq '=> .*/(BUILD|BUILDROOT)/|not found' "$work/loader"
done
printf 'Staged UPower daemon/client resolve the original Fedora library; exported ABI matches and no library/development payload remains.\n'
