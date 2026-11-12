#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# RPM buildroot data only. Never copy a library, executable or private input.
set -Eeuo pipefail
umask 077
[[ $# == 1 ]]
root=$(realpath -m "$1")
[[ ! -e $root && ! -L $root ]]
mkdir -p "$root"/{usr/bin,usr/lib64,usr/libexec/uke-control,usr/lib/qcom/adsp,run/systemd,run/uke-ssc,dev/dma_heap,sys/devices/soc0,proc,mnt/vendor,persist/sensors/registry,vendor/etc/sensors/config,odm/etc/sensors/config,system,tmp,var/tmp}
ln -s usr/bin "$root/bin"
ln -s usr/lib "$root/lib"
ln -s usr/lib64 "$root/lib64"
ln -s /persist "$root/mnt/vendor/persist"
ln -s /vendor "$root/system/vendor"
for path in usr/bin/flock usr/lib/ld-linux-aarch64.so.1 usr/libexec/uke-control/fastrpc-session \
 run/systemd/notify run/uke-ssc/registry.lock dev/fastrpc-adsp dev/dma_heap/system \
 vendor/etc/sensors/sns_reg_config; do
 install -m600 /dev/null "$root/$path"
done
