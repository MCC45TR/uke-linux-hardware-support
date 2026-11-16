#!/usr/bin/bash
# Demand-only Linux logical swap admission. Existing Control activates swap.
set -Eeuo pipefail
export LC_ALL=C
umask 077
fail() { printf 'UKE_SUPER_SWAP_REJECTED: %s\n' "$*" >&2;exit 1; }
[[ $(id -u) == 0 ]] || fail 'root required'
verify=false
case ${1:-} in
 --prepare) [[ $# == 1 ]] || fail 'expected --prepare';;
 --verify) [[ $# == 2 && -b $2 ]] || fail 'expected --verify and one mapped block device';verify=true;;
 *) fail 'expected --prepare or --verify DEVICE';;
esac
here=$(cd -- "$(dirname -- "$0")" && pwd)
# shellcheck source=early-super-policy.sh
# shellcheck disable=SC1091
source "$here/early-super-policy.sh"
mode=$(uke_early_super_policy "$(cat /proc/cmdline)") || fail 'invalid running boot profile'
if [[ $mode == none && $verify == false ]];then exit 0;fi
[[ $mode == userdata ]] || fail 'logical swap requires the explicit normal seed4 profile'
# Keep the selected descriptor open across metadata admission and DM creation.
# shellcheck source=select-uke-super.sh
# shellcheck disable=SC1091
source "$here/select-uke-super.sh"
uke_open_physical_super
if [[ $verify == true ]] || dmsetup info linux_swap >/dev/null 2>&1;then
    bash "$here/map-fastboot-swap.sh" --verify /proc/self/fd/3
else
    bash "$here/map-fastboot-swap.sh" /proc/self/fd/3
fi
if [[ $verify == true ]];then
    [[ $(readlink -e "$2") == "$(readlink -e /dev/mapper/linux_swap)" ]] || fail 'candidate differs from the verified logical swap device'
fi
