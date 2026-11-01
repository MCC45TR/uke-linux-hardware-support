#!/bin/bash
# Host-only privacy normalization for dracut 112 systemd-sysusers output.
# No password, account, service or target security policy is enabled here.
set -euo pipefail
[[ $# == 1 && -d $1 ]]
base=$(realpath "$1")
[[ -s $base/etc/passwd && -s $base/etc/group ]] || { echo 'Missing generated account database' >&2; exit 1; }
for table in shadow gshadow; do
 path=$base/etc/$table
 [[ -e $path ]] || continue
 [[ -f $path && ! -L $path ]]
 # A hash, empty password or unexpected record fails before any deletion.
 # All referenced accounts must already exist in the generated base tables.
 if [[ $table == shadow ]]; then
  # systemd 261 explicitly uses !unprovisioned for a new root account;
  # src/sysusers/sysusers.c and src/basic/user-util.h define this locked marker.
  awk -F: 'NR==FNR {accounts[$1]=1; next} NF!=9 {print "Unexpected generated shadow record shape" > "/dev/stderr"; bad=1} !($1 in accounts) {print "Unknown generated shadow account" > "/dev/stderr"; bad=1} $2 !~ /^(!|!!|\*|!\*)$/ && !($1=="root" && $2=="!unprovisioned") {print "Generated shadow password is not an admitted locked marker" > "/dev/stderr"; bad=1} END {exit bad}' "$base/etc/passwd" "$path"
 else
  awk -F: 'NR==FNR {accounts[$1]=1; next} NF!=4 {print "Unexpected generated gshadow record shape" > "/dev/stderr"; bad=1} !($1 in accounts) {print "Unknown generated gshadow account" > "/dev/stderr"; bad=1} $2 !~ /^(!|!!|\*|!\*)$/ {print "Generated gshadow password is not an admitted locked marker" > "/dev/stderr"; bad=1} $3!="" || $4!="" {print "Generated gshadow has account memberships" > "/dev/stderr"; bad=1} END {exit bad}' "$base/etc/group" "$path"
 fi
 [[ -z $(awk -F: '$1=="builder" {print "unexpected-build-account"}' "$path") ]]
 rm -- "$path"
done
# Leave passwd/group, their locked references and every service untouched.
[[ ! -e $base/etc/shadow && ! -e $base/etc/gshadow ]]
