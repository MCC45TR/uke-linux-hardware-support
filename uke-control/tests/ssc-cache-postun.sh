#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Actual expanded RPM postun block, public mock manager and disposable vectors.
set -Eeuo pipefail
data_only=false
if [[ $# == 2 && $1 == --data-only ]];then data_only=true;shift
else test ! -e /etc/senemos/kernel-update.conf;fi
[[ $# == 1 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin" "$work/units"
if [[ $data_only == true ]];then sed 's/%%/%/g' "$1" > "$work/spec"
else rpmspec -P "$1" > "$work/spec";fi
awk '
 $0=="# UKE_FASTRPC_MOUNT_POSTUN_BEGIN"{copy=1;begin++;next}
 $0=="# UKE_FASTRPC_MOUNT_POSTUN_END"{copy=0;end++;next}
 copy{print}
 END{if(begin!=1||end!=1)exit 1}' "$work/spec" > "$work/cleanup.raw"
node --input-type=module - "$work" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';
const w=process.argv[2];let text=fs.readFileSync(w+'/cleanup.raw','utf8');
for(const [a,b,count]of [['/run/systemd/system',w+'/manager',1],['/usr/lib/systemd/system/',w+'/units/',1],['/usr/bin/systemctl',w+'/bin/systemctl',2]]){assert.equal(text.split(a).length,count+1);text=text.replaceAll(a,b);}
fs.writeFileSync(w+'/cleanup',text);
JS
sh -n "$work/cleanup"
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_POSTUN_WORK/calls"
if [[ $# == 4 && $1 == show && $2 == --property=ActiveState && $3 == --value ]];then
 case $4 in uke-ssc-sensors.service|uke-ssc-root.service|uke-ssc-cache.service|'run-uke\x2dssc-fedora.mount') :;; *) exit 92;; esac
 cat "$UKE_POSTUN_WORK/active";exit 0
fi
[[ $# == 5 && $1 == stop && $2 == uke-ssc-sensors.service && $3 == uke-ssc-root.service && $4 == uke-ssc-cache.service && $5 == 'run-uke\x2dssc-fedora.mount' ]]
[[ ${UKE_POSTUN_FAIL:-no} == no ]] || exit 71
# Model the stopped-reader cascade and the resulting unmount, never host data.
printf 'inactive\n' > "$UKE_POSTUN_WORK/active"
rm -f "$UKE_POSTUN_WORK/new-generation"
SYSTEMD
chmod 700 "$work/bin/systemctl"
export UKE_POSTUN_WORK=$work
unit=$work/units/'run-uke\x2dssc-fedora.mount'
cases=0
for online in yes no;do
 for retained in yes no;do
  for count in 0 1 2;do
   mkdir -p "$work/manager"
   [[ $online == yes ]] || rmdir "$work/manager"
   if [[ $retained == yes ]];then touch "$unit";else rm -f "$unit";fi
   printf 'warm private generation\n' > "$work/new-generation"
   printf 'accepted old generation\n' > "$work/old-generation"
   printf 'active\n' > "$work/active"
   printf 'static disabled masked\n' > "$work/preferences"
   : > "$work/calls"
   sh "$work/cleanup" "$count"
   if [[ $online == yes && $retained == no ]];then
    [[ ! -e $work/new-generation && $(cat "$work/active") == inactive && $(wc -l < "$work/calls") == 5 ]]
   else
    [[ $(cat "$work/new-generation") == 'warm private generation' && $(cat "$work/active") == active && ! -s $work/calls ]]
   fi
   [[ $(cat "$work/old-generation") == 'accepted old generation' ]]
   [[ $(cat "$work/preferences") == 'static disabled masked' ]]
   ((cases+=1))
  done
 done
done
mkdir -p "$work/manager";rm -f "$unit"
for state in active reloading refreshing activating deactivating maintenance failed;do
 printf '%s\n' "$state" > "$work/active"
 printf 'warm private generation\n' > "$work/new-generation";: > "$work/calls"
 sh "$work/cleanup" 1
 [[ ! -e $work/new-generation && $(cat "$work/active") == inactive && $(grep -c '^stop ' "$work/calls") == 1 ]]
done
: > "$work/calls"
sh "$work/cleanup" 1
! grep -q '^stop ' "$work/calls"
printf 'unknown\n' > "$work/active"
if sh "$work/cleanup" 1;then exit 1;fi
mkdir -p "$work/manager";rm -f "$unit"
printf 'active\n' > "$work/active"
printf 'warm private generation\n' > "$work/new-generation"
if UKE_POSTUN_FAIL=yes sh "$work/cleanup" 1;then exit 1;fi
[[ $(cat "$work/new-generation") == 'warm private generation' ]]
printf 'SSC postun:%s online/offline retained/removed provider cases,seven stop states,idempotent inactive removal,unknown-state/failed-stop refusals;warm generation/preferences/old recovery retained.\n' "$cases"
