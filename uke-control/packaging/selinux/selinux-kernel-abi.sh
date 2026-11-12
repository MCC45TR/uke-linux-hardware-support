#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Demand-only RPM integration; no service, timer or hardware operation.
set -Eeuo pipefail
trap 'printf "SELinux integration failed: phase=%s line=%s\n" "${mode:-arguments}" "$LINENO" >&2' ERR
export LC_ALL=C
umask 077
root=/
selected=
while (($#)); do
    case $1 in
        --root) [[ $# -ge 2 ]]; root=$2; shift 2;;
        --profile) [[ $# -ge 2 && $2 =~ ^(targeted|minimum|mls)$ ]]; selected=$2; shift 2;;
        capture|stage|finish|clean) mode=$1; shift; break;;
        *) echo 'Invalid SELinux integration argument' >&2; exit 2;;
    esac
done
[[ $# == 0 && ${mode:-} =~ ^(capture|stage|finish|clean)$ ]]
root=$(realpath -e -- "$root")
if [[ $root == / ]]; then
    [[ $EUID == 0 ]]
    prefix=
    offline=false
else
    [[ -d $root && -O $root ]]
    prefix=$root
    offline=true
fi
script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
composer=$script_dir/compose-kernel-abi.awk
state=$prefix/run/uke-selinux-migration
module=uke_kernel_compat
marker='; uke-control managed kernel ABI compatibility v1'
profiles=()
if [[ -n $selected ]]; then profiles=("$selected"); else profiles=(targeted minimum mls); fi

private_state() {
    [[ ! -L $prefix/run ]]
    if [[ ! -d $prefix/run ]]; then mkdir -m755 -- "$prefix/run"; fi
    [[ ! -L $state ]]
    mkdir -p -- "$state"
    [[ -d $state && -O $state && $(stat -c %a -- "$state") == 700 ]]
}
clean_state() {
    [[ -e $state ]] || return 0
    [[ -d $state && ! -L $state && -O $state && $(stat -c %a -- "$state") == 700 ]]
    for profile in targeted minimum mls; do
        file=$state/$profile.before.35
        if [[ -e $file ]]; then [[ -f $file && ! -L $file && -O $file ]]; rm -f -- "$file"; fi
    done
    rmdir -- "$state" 2>/dev/null || :
}
if [[ $mode == clean ]]; then clean_state; exit 0; fi
if [[ $mode == capture ]]; then
    for profile in "${profiles[@]}"; do
        binary=$prefix/var/lib/selinux/$profile/active/policy.kern
        [[ -f $binary && ! -L $binary ]] || continue
        private_state
        file=$state/$profile.before.35
        [[ ! -L $file && (! -e $file || -O $file) ]]
        temporary=$(mktemp "$state/$profile.capture.XXXXXXXX")
        if ! cp -- "$binary" "$temporary"; then rm -f -- "$temporary"; exit 1; fi
        chmod 600 -- "$temporary"
        mv -f -- "$temporary" "$file"
    done
    exit 0
fi

work_parent=$prefix/run
[[ ! -L $work_parent ]]
if [[ ! -d $work_parent ]]; then mkdir -m755 -- "$work_parent"; fi
[[ -d $work_parent ]]
work=$(mktemp -d "$work_parent/uke-selinux-work.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
unpack_cil() {
    local input=$1 output=$2
    [[ -f $input && ! -L $input ]]
    if [[ $(head -c 3 -- "$input") == BZh ]]; then bzip2 -dc -- "$input" > "$output";
    else cat -- "$input" > "$output"; fi
}
run_semodule() {
    semodule -n -p "$root" -s "$profile" "$@"
}
guard_module() {
    local dir=$store/active/modules/400/$module
    run_semodule --list-modules=full > "$work/modules.list"
    if awk -v n="$module" '$2==n && $1!=400{found=1}END{exit !found}' "$work/modules.list"; then
        echo 'Conflicting SELinux ABI module priority' >&2; return 1
    fi
    if [[ -e $dir ]]; then
        [[ -d $dir && ! -L $dir && -O $dir && -f $dir/lang_ext && ! -L $dir/lang_ext && $(cat "$dir/lang_ext") == cil ]]
        unpack_cil "$dir/hll" "$work/installed.cil"
        [[ $(head -n 1 "$work/installed.cil") == "$marker" ]] || {
            echo 'Existing SELinux ABI module is not Control-managed' >&2; return 1;
        }
    fi
}
active_profile=
if ! $offline && selinuxenabled; then
    active_profile=$(awk -F= '/^[[:space:]]*SELINUXTYPE[[:space:]]*=/ {
        v=$2;sub(/[[:space:]]*#.*/,"",v);gsub(/["\047[:space:]]/,"",v);print v
    }' /etc/selinux/config)
    [[ $active_profile =~ ^(targeted|minimum|mls)$ ]]
fi
handled_active=false
for profile in "${profiles[@]}"; do
    store=$prefix/var/lib/selinux/$profile
    base=$store/active/modules/100/base/cil
    [[ -f $base && ! -L $base ]] || continue
    [[ ! -L $store && ! -L $store/active && ! -L $store/active/modules ]]
    guard_module
    unpack_cil "$base" "$work/base.cil"
    body=$store/active/policy.kern
    if [[ $mode == stage && -f $state/$profile.before.35 && ! -L $state/$profile.before.35 ]]; then
        body=$state/$profile.before.35
    elif [[ $mode == stage ]] && ! $offline && [[ $profile == "$active_profile" ]] && selinuxenabled; then
        body=/sys/fs/selinux/policy
    fi
    [[ -r $body && ! -L $body ]]
    # The first link resolves a possibly stale pre-update domain snapshot.
    # No reload occurs here. The second pass uses the resulting actual store,
    # so additions/removals and local module preferences reach a fixed point.
    for pass in 1 2; do
        checkpolicy -b -M -C -o "$work/body.cil" "$body" > "$work/decompile.log" 2>&1 || {
            echo "Cannot inspect linked SELinux policy for $profile" >&2; exit 1;
        }
        gawk -f "$composer" "$work/base.cil" "$work/body.cil" > "$work/$module.cil"
        if grep -q '^[[:space:]]*(' "$work/$module.cil"; then
            run_semodule -X 400 -i "$work/$module.cil"
        elif [[ -d $store/active/modules/400/$module ]]; then
            run_semodule -X 400 -r "$module"
        fi
        body=$store/active/policy.kern
    done
    if [[ $mode == finish ]]; then run_semodule -B; fi
    if [[ $profile == "$active_profile" ]]; then handled_active=true; fi
done
if [[ $mode == finish ]]; then
    if ! $offline && selinuxenabled; then
        $handled_active || { echo 'Active SELinux policy store is unavailable' >&2; exit 1; }
        enforcement=$(getenforce)
        load_policy
        [[ $(getenforce) == "$enforcement" ]]
    fi
    clean_state
fi
