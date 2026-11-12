#!/usr/bin/bash
# Public software-only preflight: notify socket in restricted RootDirectory.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
[[ $EUID == 0 && $# == 1 && -d /run/systemd/system ]]
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
mkdir -m700 "$output"
work=$(mktemp -d /run/uke-fastrpc-notify.XXXXXXXX)
unit=$(basename "$work")
mounted=false
cleanup(){
 systemctl stop "$unit.service" >/dev/null 2>&1 || :
 systemctl reset-failed "$unit.service" >/dev/null 2>&1 || :
 if [[ $mounted == true ]];then umount "$work/noswap";mounted=false;fi
 find "$work" -depth -delete
}
trap cleanup EXIT
mkdir -p "$work"/{root/usr/bin,root/usr/lib64,root/usr/lib,root/proc,root/sys,root/run/systemd,root/run/uke-ssc,noswap}
ln -s usr/bin "$work/root/bin"
ln -s usr/lib "$work/root/lib"
ln -s usr/lib64 "$work/root/lib64"
for file in flock systemd-notify sleep;do touch "$work/root/usr/bin/$file";done
touch "$work/root/usr/lib/ld-linux-aarch64.so.1"
touch "$work/root/run/systemd/notify"
touch "$work/root/run/uke-ssc/registry.lock" "$work/registry.lock"
mount -t tmpfs -o size=16M,nr_inodes=1024,mode=0700,noswap tmpfs "$work/noswap"
mounted=true
findmnt --mountpoint "$work/noswap" --noheadings --output FSTYPE,OPTIONS > "$output/mount-options.txt"
grep -Eq '^tmpfs[[:space:]].*(^|,)noswap(,|$)' "$output/mount-options.txt"
printf 'public RAM-only vector\n' > "$work/noswap/vector"
status=0
systemd-run --wait --pipe --unit="$unit" --setenv=LC_ALL=C \
 -p Type=notify -p NotifyAccess=main -p "RootDirectory=$work/root" \
 -p 'BindReadOnlyPaths=/usr/lib64 /usr/lib/ld-linux-aarch64.so.1 /usr/bin/flock /usr/bin/systemd-notify /usr/bin/sleep /run/systemd/notify' \
 -p "BindPaths=$work/registry.lock:/run/uke-ssc/registry.lock" -p ReadWritePaths=/run/uke-ssc/registry.lock \
 -p ProtectSystem=strict -p ProtectHome=yes -p PrivateDevices=yes \
 -p 'TemporaryFileSystem=/sys:ro /proc:ro' -p MountAPIVFS=no -p PrivateMounts=yes \
 -p NoNewPrivileges=yes -p CapabilityBoundingSet= \
 -p RestrictAddressFamilies=AF_UNIX -p MemoryDenyWriteExecute=yes \
 -p RuntimeMaxSec=10 -p TimeoutStartSec=8 -p TimeoutStopSec=3 \
 -- /usr/bin/flock --shared --no-fork /run/uke-ssc/registry.lock /usr/bin/systemd-notify --pid=self --ready --exec ';' /usr/bin/sleep 2 \
 < /dev/null > "$output/result.log" 2>&1 || status=$?
systemctl show --property=Result --property=ExecMainStatus --property=ActiveState --property=SubState "$unit.service" > "$output/unit-result.txt" 2>&1 || :
[[ $status == 0 ]]
grep -Fqx 'public RAM-only vector' "$work/noswap/vector"
cleanup
trap - EXIT
[[ ! -e $work ]]
printf 'Type=notify/NotifyAccess=main passed in minimal RootDirectory with empty proc/sys,explicit notify socket,AF_UNIX only and empty capabilities;noswap16MiB tmpfs verified and removed.\n' >> "$output/result.log"
touch "$output/complete"
cat "$output/result.log"
