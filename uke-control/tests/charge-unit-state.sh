#!/usr/bin/bash
# Execute actual expanded RPM fragments with fake systemd and private files.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
spec=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin" "$work/manager"
rpmspec -P "$spec" > "$work/spec"
for phase in SAVE RESTORE;do
 awk -v phase="$phase" '
  $0=="# UKE_CONTROL_CHARGE_"phase"_BEGIN" {copy=1;found++;next}
  $0=="# UKE_CONTROL_CHARGE_"phase"_END" {copy=0;done++;next}
  copy {print}
  END{if(found!=1 || done!=1)exit 1}' "$work/spec" > "$work/$phase.raw"
 sed -e "s|/run/uke-control-charge-migration|$work/state|g" \
  -e "s|/run/systemd/system|$work/manager|g" \
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
[[ ${UKE_CHARGE_OLD:-yes} == yes ]] || exit 1
printf 'uke-charge-control'
RPM
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_CHARGE_CALLS"
case $1 in
 is-enabled)
  case $2 in
   uke-charge-control.service)
    if [[ ${UKE_CHARGE_PHASE:-save} == restore ]];then printf '%s\n' "${UKE_CHARGE_CURRENT_STATE:-disabled}"
    else printf '%s\n' "${UKE_CHARGE_SAVED_STATE:-static}";fi;;
   uke-charge-automatic.service) printf 'enabled-runtime\n';;
   uke-charge-protection.service) printf 'disabled\n';;
   *) exit 91;;
  esac;;
 show)
  case ${*: -1} in
   uke-charge-control.service) printf '%s\n' "${UKE_CHARGE_SAVED_ACTIVE:-inactive}";;
   uke-charge-automatic.service|uke-charge-protection.service) printf 'active\n';;
   *) exit 92;;
  esac;;
 daemon-reload|--no-reload|start) :;;
 *) exit 93;;
esac
SYSTEMD
chmod 700 "$work/bin/"*
export UKE_CHARGE_CALLS=$work/calls
bash "$work/SAVE"
[[ $(stat -c %a "$work/state") == 700 && $(stat -c %a "$work/state/units") == 600 ]]
cp "$work/state/units" "$work/original"
if bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
cmp "$work/original" "$work/state/units"
: > "$work/calls"
UKE_CHARGE_PHASE=restore bash "$work/RESTORE"
[[ ! -e $work/state ]]
grep -Fx 'start uke-charge-automatic.service' "$work/calls"
grep -Fx 'start uke-charge-protection.service' "$work/calls"
grep -Fx -- '--no-reload enable --runtime uke-charge-automatic.service' "$work/calls"
! grep -Eq '(start|enable).*uke-charge-control[.]service' "$work/calls"
bash "$work/RESTORE"
UKE_CHARGE_OLD=no bash "$work/SAVE"
[[ ! -e $work/state ]]
for enabled in enabled enabled-runtime disabled static indirect masked masked-runtime;do
 active=active
 case $enabled in masked|masked-runtime) active=inactive;;esac
 UKE_CHARGE_SAVED_STATE=$enabled UKE_CHARGE_SAVED_ACTIVE=$active bash "$work/SAVE"
 : > "$work/calls"
 UKE_CHARGE_PHASE=restore bash "$work/RESTORE"
 case $enabled in
  enabled) grep -Fx -- '--no-reload enable uke-charge-control.service' "$work/calls";;
  enabled-runtime) grep -Fx -- '--no-reload enable --runtime uke-charge-control.service' "$work/calls";;
  masked) grep -Fx -- '--no-reload mask uke-charge-control.service' "$work/calls";;
  masked-runtime) grep -Fx -- '--no-reload mask --runtime uke-charge-control.service' "$work/calls";;
 esac
 if [[ $active == active ]];then grep -Fx 'start uke-charge-control.service' "$work/calls";else ! grep -Fx 'start uke-charge-control.service' "$work/calls";fi
done
UKE_CHARGE_SAVED_STATE=enabled UKE_CHARGE_SAVED_ACTIVE=active bash "$work/SAVE"
: > "$work/calls"
UKE_CHARGE_PHASE=restore UKE_CHARGE_CURRENT_STATE=masked bash "$work/RESTORE"
! grep -Eq '(start|enable|unmask).*uke-charge-control[.]service' "$work/calls"
for active in activating deactivating unknown;do
 if UKE_CHARGE_SAVED_ACTIVE=$active bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -e $work/state ]]
done
mkdir "$work/other"
ln -s "$work/other" "$work/state"
for phase in SAVE RESTORE;do
 if bash "$work/$phase" > "$work/refusal" 2>&1;then exit 1;fi
done
[[ ! -e $work/other/units ]]
rm "$work/state"
for alteration in foreign duplicate malformed future stale mode hardlink symlink;do
 bash "$work/SAVE"
 case $alteration in
  foreign) printf 'other.service active enabled\n' >> "$work/state/units";;
  duplicate) tail -n1 "$work/state/units" >> "$work/state/units";;
  malformed) printf 'invalid\n' > "$work/state/units";;
  future) sed -i "1s/[0-9]*$/$(($(date +%s)+60))/" "$work/state/units";;
  stale) sed -i "1s/[0-9]*$/$(($(date +%s)-3601))/" "$work/state/units";;
  mode) chmod 644 "$work/state/units";;
  hardlink) ln "$work/state/units" "$work/linked";;
  symlink) mv "$work/state/units" "$work/linked";ln -s "$work/linked" "$work/state/units";;
 esac
 : > "$work/calls"
 if UKE_CHARGE_PHASE=restore bash "$work/RESTORE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -s $work/calls ]]
 rm -f "$work/state/units" "$work/linked"
 rmdir "$work/state"
done
printf 'Actual charge RPM migration: seven enable states,active/inactive,new masks,fresh/idempotent and malformed/link/stale/transition refusals passed; fake systemd only.\n'
