#!/bin/bash
# Host-only upstream dracut interface; no project interpreter is added to the target.
# shellcheck disable=SC2154
check() { require_binaries /usr/libexec/senemos-uke/uke-boot-guard; }
depends() { echo systemd; }
installkernel() { instmods nvmem_reboot_mode nvmem_qcom_spmi_sdam; }
install() {
    inst_multiple /usr/libexec/senemos-uke/uke-boot-guard
    inst_simple "$systemdsystemunitdir/uke-boot-guard.service" "$systemdsystemunitdir/uke-boot-guard.service"
    inst_simple /usr/lib/modules-load.d/uke-reboot.conf /usr/lib/modules-load.d/uke-reboot.conf
    mkdir -p "$initdir$systemdsystemunitdir/initrd.target.wants"
    ln -s ../uke-boot-guard.service "$initdir$systemdsystemunitdir/initrd.target.wants/uke-boot-guard.service"
}
