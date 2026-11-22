#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
unit=src/units/uke-boot-guard.service
for setting in 'DefaultDependencies=no' 'IgnoreOnIsolate=yes' \
    'Conflicts=shutdown.target initrd-switch-root.target' \
    'Before=shutdown.target initrd-switch-root.target' \
    'FailureAction=reboot-force' 'RebootArgument=bootloader' \
    'ConditionFirmware=device-tree-compatible(xiaomi,uke)' 'RuntimeMaxSec=185'; do
    grep -Fx "$setting" "$unit" >/dev/null
done
! grep -Eq '^(RemainAfterExit=yes|Restart=|ExecStart=.*(bash|sh -c))' "$unit"
grep -F 'initrd.target.wants/uke-boot-guard.service' src/dracut/93uke-boot-guard/module-setup.sh >/dev/null
grep -Fx 'nvmem_reboot_mode' src/profiles/uke-reboot.conf >/dev/null
grep -F 'sysinit.target.wants/uke-boot-guard.service' packaging/uke-boot-integration.spec >/dev/null
printf '%s\n' 'Initramfs/root startup links, early isolation and failure-action contract passed'
