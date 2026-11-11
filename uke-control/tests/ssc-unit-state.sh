#!/usr/bin/bash
# Run the actual expanded RPM migration with private records and fake systemd.
set -Eeuo pipefail
data_only=false
if [[ $# == 2 && $1 == --data-only ]];then data_only=true;shift
else test ! -e /etc/senemos/kernel-update.conf;fi
[[ $# == 1 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin" "$work/manager"
if [[ $data_only == true ]];then
 sed 's/%%/%/g' "$(realpath "$1")" > "$work/spec"
else rpmspec -P "$(realpath "$1")" > "$work/spec";fi
for phase in SAVE RESTORE;do
 awk -v phase="$phase" '
  $0=="# UKE_CONTROL_SSC_"phase"_BEGIN" {copy=1;found++;next}
  $0=="# UKE_CONTROL_SSC_"phase"_END" {copy=0;done++;next}
  copy {print}
  END{if(found!=1 || done!=1)exit 1}' "$work/spec" > "$work/$phase.raw"
 sed -e "s|/run/uke-control-ssc-migration|$work/state|g" -e "s|/run/systemd/system|$work/manager|g" \
  -e "s|/usr/bin/id|$work/bin/id|g" -e "s|/usr/bin/stat|$work/bin/stat|g" \
  -e "s|/usr/bin/systemctl|$work/bin/systemctl|g" -e "s|/usr/bin/rpm|$work/bin/rpm|g" \
  "$work/$phase.raw" > "$work/$phase"
 sh -n "$work/$phase"
done
cat > "$work/bin/id" <<'ID'
#!/usr/bin/bash
printf '0\n'
ID
cat > "$work/bin/stat" <<'STAT'
#!/usr/bin/bash
if [[ $1 == -c && ($2 == '%u:%g:%a' || $2 == '%u:%g:%a:%h') ]];then
 value=$(/usr/bin/stat "$@");printf '0:0:%s\n' "${value#*:*:}"
else exec /usr/bin/stat "$@";fi
STAT
cat > "$work/bin/rpm" <<'RPM'
#!/usr/bin/bash
[[ ${UKE_SSC_OLD:-yes} == yes ]] || exit 1
printf 'uke-ssc-fileservice'
RPM
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_SSC_CALLS"
case $1 in
 is-enabled)
  case $2 in uke-ssc-cache.service|uke-ssc-root.service|uke-ssc-sensors.service|iio-sensor-proxy.service) :;; *) exit 91;; esac
  if [[ ${UKE_SSC_PHASE:-save} == restore ]];then printf '%s\n' "${UKE_SSC_CURRENT_STATE:-disabled}"
  else printf '%s\n' "${UKE_SSC_SAVED_STATE:-disabled}";fi;;
 show) case ${*: -1} in uke-ssc-cache.service|uke-ssc-root.service|uke-ssc-sensors.service|iio-sensor-proxy.service) :;; *) exit 92;; esac;printf '%s\n' "${UKE_SSC_SAVED_ACTIVE:-inactive}";;
 daemon-reload|--no-reload|start) :;;
 *) exit 93;;
esac
SYSTEMD
chmod 700 "$work/bin/"*
export UKE_SSC_CALLS=$work/calls
for enabled in enabled enabled-runtime disabled static indirect masked masked-runtime;do
 for active in inactive active;do
  case $enabled:$active in masked*:active) continue;;esac
  UKE_SSC_SAVED_STATE=$enabled UKE_SSC_SAVED_ACTIVE=$active bash "$work/SAVE"
  [[ $(stat -c %a "$work/state") == 700 && $(stat -c %a "$work/state/units") == 600 ]]
  cp "$work/state/units" "$work/original"
  if bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
  cmp "$work/original" "$work/state/units"
  : > "$work/calls"
  UKE_SSC_PHASE=restore bash "$work/RESTORE"
  [[ ! -e $work/state ]]
  case $enabled in
   enabled) grep -Fxq -- '--no-reload enable uke-ssc-root.service' "$work/calls";;
   enabled-runtime) grep -Fxq -- '--no-reload enable --runtime uke-ssc-root.service' "$work/calls";;
   masked) grep -Fxq -- '--no-reload mask uke-ssc-root.service' "$work/calls";;
   masked-runtime) grep -Fxq -- '--no-reload mask --runtime uke-ssc-root.service' "$work/calls";;
   disabled|static|indirect) ! grep -Eq '(enable|mask)' "$work/calls";;
  esac
  if [[ $active == active ]];then [[ $(grep -c '^start ' "$work/calls") == 4 ]];grep -Fxq 'start uke-ssc-root.service' "$work/calls";else ! grep -Fxq 'start uke-ssc-root.service' "$work/calls";fi
 done
done
bash "$work/RESTORE"
UKE_SSC_OLD=no bash "$work/SAVE"
[[ ! -e $work/state ]]
UKE_SSC_SAVED_STATE=enabled UKE_SSC_SAVED_ACTIVE=active bash "$work/SAVE"
: > "$work/calls"
UKE_SSC_PHASE=restore UKE_SSC_CURRENT_STATE=masked bash "$work/RESTORE"
! grep -Eq '(start|enable|unmask)' "$work/calls"
for active in activating deactivating unknown;do
 if UKE_SSC_SAVED_ACTIVE=$active bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -e $work/state ]]
done
mkdir "$work/other"
ln -s "$work/other" "$work/state"
for phase in SAVE RESTORE;do if bash "$work/$phase" > "$work/refusal" 2>&1;then exit 1;fi;done
[[ ! -e $work/other/units ]]
rm "$work/state"
for alteration in foreign duplicate malformed future stale mode hardlink symlink;do
 bash "$work/SAVE"
 case $alteration in
  foreign) printf 'uke-battery.service active enabled\n' >> "$work/state/units";;
  duplicate) tail -n1 "$work/state/units" >> "$work/state/units";;
  malformed) printf 'invalid\n' > "$work/state/units";;
  future) sed -i "1s/[0-9]*$/$(($(date +%s)+60))/" "$work/state/units";;
  stale) sed -i "1s/[0-9]*$/$(($(date +%s)-3601))/" "$work/state/units";;
  mode) chmod 644 "$work/state/units";;
  hardlink) ln "$work/state/units" "$work/linked";;
  symlink) mv "$work/state/units" "$work/linked";ln -s "$work/linked" "$work/state/units";;
 esac
 : > "$work/calls"
 if UKE_SSC_PHASE=restore bash "$work/RESTORE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -s $work/calls ]]
 rm -f "$work/state/units" "$work/linked"
 rmdir "$work/state"
done
! grep -Fq uke-battery.service "$work/RESTORE"
printf 'SSC four-unit migration: seven enable states,active/inactive,new masks,fresh/idempotent and malformed/link/stale/transition refusals pass;fake systemd only.\n'
