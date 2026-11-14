#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Transaction-only integration; no daemon or hardware access.
set -Eeuo pipefail
export LC_ALL=C
umask 077
root=/
selected=
while (($#));do
    case $1 in
        --root) [[ $# -ge 2 ]];root=$2;shift 2;;
        --profile) [[ $# -ge 2 && $2 =~ ^(targeted|minimum|mls)$ ]];selected=$2;shift 2;;
        install|remove) mode=$1;shift;break;;
        *) echo 'Invalid Motion policy argument' >&2;exit 2;;
    esac
done
[[ $# == 0 && ${mode:-} =~ ^(install|remove)$ ]]
root=$(realpath -e -- "$root")
offline=true
prefix=$root
if [[ $root == / ]];then [[ $EUID == 0 ]];offline=false;prefix=;
else [[ -d $root && -O $root ]];fi
module=uke_control_motion
marker='; uke-control managed Motion1 policy v1'
payload=$prefix/usr/share/selinux/packages/$module.cil
profiles=(targeted minimum mls)
[[ -z $selected ]] || profiles=("$selected")
[[ ! -L $prefix/run ]]
[[ -d $prefix/run ]] || mkdir -m755 -- "$prefix/run"
work=$(mktemp -d "$prefix/run/uke-motion-policy.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
unpack_cil(){
    [[ -f $1 && ! -L $1 ]]
    if [[ $(head -c 3 -- "$1") == BZh ]];then bzip2 -dc -- "$1" > "$2";
    else cat -- "$1" > "$2";fi
}
run_semodule(){ timeout -k 15s 300s semodule -n -p "$root" -s "$profile" "$@"; }
active=
if ! $offline && selinuxenabled;then
    active=$(awk -F= '/^[[:space:]]*SELINUXTYPE[[:space:]]*=/ {v=$2;sub(/[[:space:]]*#.*/,"",v);gsub(/["\047[:space:]]/,"",v);print v}' /etc/selinux/config)
    [[ $active =~ ^(targeted|minimum|mls)$ ]]
fi
if [[ $mode == install ]];then
    [[ -f $payload && ! -L $payload && -O $payload && $(head -n 1 -- "$payload") == "$marker" ]]
fi
for profile in "${profiles[@]}";do
    store=$prefix/var/lib/selinux/$profile
    [[ -f $store/active/modules/100/base/cil ]] || continue
    for path in "$store" "$store/active" "$store/active/modules" "$store/active/modules/400";do [[ ! -L $path ]];done
    run_semodule --list-modules=full > "$work/modules.list"
    if awk -v name="$module" '$2==name&&$1!=400{found=1}END{exit !found}' "$work/modules.list";then
        echo 'Refusing a conflicting Motion policy priority' >&2;exit 1
    fi
    installed=$store/active/modules/400/$module
    disabled=false
    if [[ -e $installed ]];then
        [[ -d $installed && ! -L $installed && -O $installed && -f $installed/lang_ext && ! -L $installed/lang_ext && $(cat "$installed/lang_ext") == cil ]]
        unpack_cil "$installed/cil" "$work/installed.cil"
        [[ $(head -n 1 "$work/installed.cil") == "$marker" ]] || { echo 'Refusing a foreign Motion policy module' >&2;exit 1; }
        if awk -v name="$module" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/modules.list";then disabled=true;fi
    fi
    if [[ $mode == install ]];then
        args=(-X 400 -i "$payload")
        # Preserve an explicit user disable in the same libsemanage transaction.
        if $disabled;then args+=(-d "$module");fi
        run_semodule "${args[@]}"
    elif [[ -d $installed ]];then run_semodule -X 400 -r "$module";
    else continue;fi
    if ! $offline && [[ $profile == "$active" ]];then
        enforcement=$(getenforce)
        timeout -k 15s 300s semodule -p / -s "$profile" -B
        [[ $(getenforce) == "$enforcement" ]]
    fi
done
