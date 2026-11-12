#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Target software lifecycle fixture: public tmpfs vector and original utilities.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
[[ $EUID == 0 && $# == 3 && $(getenforce) == Enforcing ]]
[[ $(uname -r) == 7.3.3-24-unstable && $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-control) == 2.17.0-6.fc46 ]]
spec=$(realpath "$1");output=$(realpath -m "$2")
[[ ! -e $output && ! -L $output ]]
exec 9>"$3";flock --exclusive --nonblock 9
mkdir -m700 "$output"
work=$(mktemp -d /run/uke-ssc-postun.XXXXXXXX)
name=$(basename "$work")
mount_unit=$(systemd-escape --path --suffix=mount "$work/cache")
cache=$name-cache.service;root=$name-root.service;sensors=$name-sensors.service
owned=("$sensors" "$root" "$cache" "$mount_unit")
cleanup() {
 local result=$?
 trap - EXIT
 systemctl stop "${owned[@]}" >/dev/null 2>&1 || :
 for unit in "${owned[@]}";do rm -f -- "/run/systemd/system/$unit";done
 systemctl daemon-reload
 systemctl reset-failed "${owned[@]}" >/dev/null 2>&1 || :
 if ! mountpoint --quiet "$work/cache";then find "$work" -depth -delete;fi
 chown -R 1000:1000 "$output"
 exit "$result"
}
trap cleanup EXIT
trap 'exit 130' INT TERM HUP
for unit in uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target;do
 systemctl show -p ActiveState -p UnitFileState "$unit" > "$output/$unit.before"
done
stat -c '%d:%i' /run/uke-ssc/state > "$output/accepted-generation.before"
mkdir -m700 "$work/cache"
cat > "/run/systemd/system/$mount_unit" <<EOF
[Unit]
Description=Uke public postun mount fixture
Before=$cache
StopWhenUnneeded=no
[Mount]
What=tmpfs
Where=$work/cache
Type=tmpfs
Options=size=16M,nr_inodes=64,mode=0700,noswap,nosuid,nodev,noexec
EOF
cat > "/run/systemd/system/$cache" <<EOF
[Unit]
Description=Uke public postun cache fixture
Requires=$mount_unit
After=$mount_unit
Before=$root
[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/true
EOF
for pair in "$root $cache" "$sensors $root";do
 read -r service parent <<< "$pair"
 cat > "/run/systemd/system/$service" <<EOF
[Unit]
Description=Uke public postun reader fixture
Requires=$parent
After=$parent
[Service]
Type=exec
ExecStart=/usr/bin/sleep 60
RuntimeMaxSec=60
EOF
done
awk '
 $0=="# UKE_FASTRPC_MOUNT_POSTUN_BEGIN"{copy=1;begin++;next}
 $0=="# UKE_FASTRPC_MOUNT_POSTUN_END"{copy=0;end++;next}
 copy{print}
 END{if(begin!=1||end!=1)exit 1}' "$spec" > "$work/cleanup.raw"
node --input-type=module - "$work" "$mount_unit" "$cache" "$root" "$sensors" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';
const [w,mount,cache,root,sensors]=process.argv.slice(2);let t=fs.readFileSync(w+'/cleanup.raw','utf8');
for(const [a,b]of [['/usr/lib/systemd/system/','/run/systemd/system/'],['run-uke'+String.fromCharCode(92)+'x2dssc-fedora.mount',mount],['uke-ssc-sensors.service',sensors],['uke-ssc-root.service',root],['uke-ssc-cache.service',cache]]){assert(t.includes(a));t=t.replaceAll(a,b);}
fs.writeFileSync(w+'/cleanup',t);
JS
systemctl daemon-reload
systemctl start "$root" "$sensors"
findmnt --mountpoint "$work/cache" --types tmpfs --options noswap --noheadings >/dev/null
printf 'public warm vector\n' > "$work/cache/vector"
printf 'public accepted vector\n' > "$work/accepted-vector"
stat -c '%d:%i' "$work/cache/vector" > "$output/warm-vector.before"
# Normal restart and retained-provider postun must not destroy warm RAM.
systemctl restart "$cache" "$root" "$sensors"
sh "$work/cleanup" 1
stat -c '%d:%i' "$work/cache/vector" | cmp - "$output/warm-vector.before"
[[ $(cat "$work/cache/vector") == 'public warm vector' ]]
for unit in "${owned[@]}";do systemctl is-active --quiet "$unit";done
# A downgrade removes the provider's unit while the mount is still active.
rm "/run/systemd/system/$mount_unit"
systemctl daemon-reload
systemctl is-active --quiet "$mount_unit"
sh "$work/cleanup" 1
! mountpoint --quiet "$work/cache"
sh "$work/cleanup" 1
[[ ! -e $work/cache/vector && $(cat "$work/accepted-vector") == 'public accepted vector' ]]
for unit in "${owned[@]}";do [[ $(systemctl show -p ActiveState --value "$unit") == inactive ]];done
for unit in uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target;do
 systemctl show -p ActiveState -p UnitFileState "$unit" | cmp - "$output/$unit.before"
done
stat -c '%d:%i' /run/uke-ssc/state | cmp - "$output/accepted-generation.before"
printf 'Original systemd:normal restart/retained postun preserve warm tmpfs inode;removed provider stops readers and unmounts;inactive repeated cleanup succeeds;accepted vector and production state retained.\n' > "$output/result.txt"
touch "$output/complete"
