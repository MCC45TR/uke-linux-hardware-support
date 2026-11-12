#!/usr/bin/bash
# COPR package-content checks; native policy composition has a separate receipt.
set -Eeuo pipefail
[[ $# == 3 && ! -e /etc/senemos/kernel-update.conf ]]
overlay=$(realpath "$1")
payload=$(realpath "$2")
spec=$(realpath "$3")
cmp "$overlay/packaging/selinux/selinux-kernel-abi.sh" "$payload/usr/libexec/uke-control/selinux-kernel-abi"
cmp "$overlay/packaging/selinux/compose-kernel-abi.awk" "$payload/usr/libexec/uke-control/compose-kernel-abi.awk"
cmp "$overlay/docs/SELINUX-ORIGINAL.md" "$payload/usr/share/doc/uke-control/SELINUX-ORIGINAL.md"
bash -n "$payload/usr/libexec/uke-control/selinux-kernel-abi"
[[ -x $payload/usr/libexec/uke-control/selinux-kernel-abi ]]
[[ ! -x $payload/usr/libexec/uke-control/compose-kernel-abi.awk ]]
rpmspec -P "$spec" | awk '
 /^%transfiletriggerun .*2000000 .*\/var\/lib\/selinux/{capture++}
 /^%filetriggerin .*2000000 .*\/var\/lib\/selinux/{stage++}
 /^%transfiletriggerin .*90000 .*\/var\/lib\/selinux/{finish++}
 END{if(capture!=1 || stage!=1 || finish!=1)exit 1}'
printf 'Control15 packaged helper/composer/docs and all three transaction hooks match; no unit or timer added.\n'
