#!/usr/bin/env bash
# Install the audited native diagnostic with DNF and inspect read-only stock.
set -Eeuo pipefail
mode=${1:---preview}
[[ $# -le 1 && ( $mode == --preview || $mode == --apply ) ]]
url=https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable:custom:cdsp-pogo-20261104/fedora-rawhide-aarch64/11260144-uke-cdsp-pogo-firmware/uke-cdsp-pogo-firmware-0.2-4.fc46.aarch64.rpm
if [[ $mode == --preview ]]; then
    printf '%s\n' 'Administrator-terminal operations:'
    printf 'dnf5 -y --setopt=localpkg_gpgcheck=True --setopt=keepcache=False install %q\n' "$url"
    printf '%s\n' '/usr/libexec/uke-cdsp-pogo-firmware --inspect-stock'
    exit 0
fi
[[ $EUID == 0 ]] || { printf '%s\n' 'Run --apply in an administrator terminal.' >&2; exit 1; }
tr '\0' '\n' < /sys/firmware/devicetree/base/compatible | grep -Fxq xiaomi,uke
current=$(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-cdsp-pogo-firmware 2>/dev/null) || current=absent
case $current in absent|0.2-3.fc46|0.2-4.fc46) ;; *) printf 'Unreviewed installed helper %s; inspect it before changing versions.\n' "$current" >&2; exit 1;; esac
/usr/bin/dnf5 -y --setopt=localpkg_gpgcheck=True --setopt=keepcache=False install "$url"
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-cdsp-pogo-firmware) == 0.2-4.fc46 ]]
rpm -V uke-cdsp-pogo-firmware
exec /usr/libexec/uke-cdsp-pogo-firmware --inspect-stock
