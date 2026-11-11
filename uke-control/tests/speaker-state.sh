#!/usr/bin/bash
# COPR only: execute actual expanded RPM migration fragments with fake systemd.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
spec=$(realpath "${1:?Actual folded spec}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin"
rpmspec -P "$spec" > "$work/spec"
for phase in SAVE RESTORE;do
 awk -v phase="$phase" '
  $0=="# UKE_CONTROL_SPEAKER_"phase"_BEGIN" {copy=1;found++;next}
  $0=="# UKE_CONTROL_SPEAKER_"phase"_END" {copy=0;done++;next}
  copy {print}
  END{if(found!=1 || done!=1)exit 1}' "$work/spec" > "$work/$phase.raw"
 sed -e "s|/run/uke-control-speaker-migration|$work/state|g" \
  -e "s|/run/systemd/system|$work/manager|g" \
  -e "s|/usr/bin/id|$work/bin/id|g" -e "s|/usr/bin/stat|$work/bin/stat|g" \
  -e "s|/usr/bin/systemctl|$work/bin/systemctl|g" -e "s|/usr/bin/rpm|$work/bin/rpm|g" \
  "$work/$phase.raw" > "$work/$phase"
 bash -n "$work/$phase"
done
mkdir "$work/manager"
cat > "$work/bin/id" <<'ID'
#!/usr/bin/bash
printf '0\n'
ID
cat > "$work/bin/stat" <<'STAT'
#!/usr/bin/bash
# Build workers are unprivileged; only UID/GID are simulated, real modes/links stay.
if [[ $1 == -c && ($2 == '%u:%g:%a' || $2 == '%u:%g:%a:%h') ]];then
 value=$(/usr/bin/stat "$@")
 printf '0:0:%s\n' "${value#*:*:}"
else exec /usr/bin/stat "$@";fi
STAT
cat > "$work/bin/rpm" <<'RPM'
#!/usr/bin/bash
[[ ${UKE_SPEAKER_OLD:-yes} == yes ]] || exit 1
printf 'uke-speaker-runtime'
RPM
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_SPEAKER_CALLS"
case $1 in
 is-enabled)
  case $2 in
   uke-speaker-provision@.service) printf '%s\n' "${UKE_SPEAKER_TEMPLATE:-static}";;
   uke-speaker-provision@controlC0.service) printf 'static\n';;
   uke-speaker-provision@controlC1.service) printf 'disabled\n';;
   uke-speaker-provision@controlC2.service) printf 'masked\n';;
   uke-speaker-provision@controlC3.service) printf 'enabled-runtime\n';;
   *) exit 91;;
  esac;;
 list-units)
  for n in 0 1 2 3;do printf 'uke-speaker-provision@controlC%s.service loaded active exited metadata\n' "$n";done;;
 show)
  case ${*: -1} in
   *controlC0.service|*controlC3.service) printf '%s\n' "${UKE_SPEAKER_ACTIVE:-active}";;
   *controlC1.service) printf 'inactive\n';;
   *controlC2.service) printf 'failed\n';;
   *) exit 92;;
  esac;;
 daemon-reload|--no-reload|start) :;;
 *) exit 93;;
esac
SYSTEMD
chmod 700 "$work/bin/"*
export UKE_SPEAKER_CALLS=$work/calls UKE_SPEAKER_OLD=yes
bash "$work/SAVE"
[[ $(stat -c %a "$work/state") == 700 && $(stat -c %a "$work/state/units") == 600 ]]
[[ $(wc -l < "$work/state/units") == 5 ]]
cp "$work/state/units" "$work/original"
if bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
cmp "$work/original" "$work/state/units"
: > "$work/calls"
bash "$work/RESTORE"
[[ ! -e $work/state ]]
grep -Fx 'start uke-speaker-provision@controlC0.service' "$work/calls"
grep -Fx 'start uke-speaker-provision@controlC3.service' "$work/calls"
grep -Fx -- '--no-reload enable --runtime uke-speaker-provision@controlC3.service' "$work/calls"
! grep -Eq '(start|enable).*controlC[12]' "$work/calls"
bash "$work/RESTORE"
UKE_SPEAKER_OLD=no bash "$work/SAVE"
[[ ! -e $work/state ]]
for active in activating deactivating unknown;do
 if UKE_SPEAKER_ACTIVE=$active bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -e $work/state ]]
done
if UKE_SPEAKER_TEMPLATE=masked bash "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
[[ ! -e $work/state ]]
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
 if bash "$work/RESTORE" > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -s $work/calls ]]
 rm -f "$work/state/units" "$work/linked"
 rmdir "$work/state"
done
printf 'Actual speaker RPM migration: bounded active/inactive/masked/runtime states, no global trigger, fresh/idempotent and stale/foreign/link/mode/transition refusals passed; fake systemd only.\n'
