#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Execute the production RPM hooks with private state and recording actors.
set -Eeuo pipefail
export LC_ALL=C
data_only=false
if [[ $# == 2 && $1 == --data-only ]];then data_only=true;shift
else [[ ! -e /etc/senemos/kernel-update.conf ]];fi
[[ $# == 1 ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mkdir "$work/bin" "$work/manager" "$work/units"
if $data_only;then sed 's/%%/%/g' "$1" > "$work/spec";
else rpmspec -P "$1" > "$work/spec";fi
for phase in SAVE RESTORE PREUN_SAVE POSTUN_RESTORE;do
 awk -v phase="$phase" '
  $0=="# UKE_CONTROL_MOTION_"phase"_BEGIN" {copy=1;found++;next}
  $0=="# UKE_CONTROL_MOTION_"phase"_END" {copy=0;done++;next}
  copy {print}
  END{if(found!=1||done!=1)exit 1}' "$work/spec" > "$work/$phase.raw"
 {
  printf '(\n'
  sed -e "s|/run/uke-control-motion-upgrade|$work/state|g" -e "s|/run/systemd/system|$work/manager|g" \
   -e "s|/usr/lib/systemd/system/uke-control-motion.service|$work/units/motion.service|g" \
   -e "s|/usr/bin/id|$work/bin/id|g" -e "s|/usr/bin/stat|$work/bin/stat|g" \
   -e "s|/usr/bin/rpm|$work/bin/rpm|g" -e "s|/usr/bin/systemctl|$work/bin/systemctl|g" \
   -e "s|/usr/bin/busctl|$work/bin/busctl|g" "$work/$phase.raw"
  printf ') || exit $?\n'
 } > "$work/$phase"
 bash -n "$work/$phase"
done
cat > "$work/bin/id" <<'ID'
#!/usr/bin/bash
printf '0\n'
ID
cat > "$work/bin/stat" <<'STAT'
#!/usr/bin/bash
set -Eeuo pipefail
value=$(/usr/bin/stat "$@")
case "$2" in
 '%u:%g:%a'|'%u:%g:%a:%h')
  if [[ ${UKE_MOTION_BAD_OWNER:-no} == yes ]];then printf '999:999:%s\n' "${value#*:*:}";
  else printf '0:0:%s\n' "${value#*:*:}";fi;;
 *) printf '%s\n' "$value";;
esac
STAT
cat > "$work/bin/rpm" <<'RPM'
#!/usr/bin/bash
[[ $1 == -qf ]] || exit 90
printf '%s\n' "${UKE_MOTION_OWNER:-uke-control}"
RPM
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf 'systemctl %s\n' "$*" >> "$UKE_MOTION_CALLS"
case $1 in
 daemon-reload) [[ $# == 1 ]];;
 is-enabled) [[ $2 == uke-control-motion.service ]];printf '%s\n' "$UKE_MOTION_ENABLED";;
 is-active)
  [[ $2 == uke-control-motion.service ]]
  if [[ -f $UKE_MOTION_RUNNING ]];then cat "$UKE_MOTION_RUNNING";else printf '%s\n' "$UKE_MOTION_ACTIVE";fi;;
 stop|start)
  [[ $2 == uke-control-motion.service && $# == 2 ]]
  [[ ${UKE_MOTION_FAIL:-none} != "$1" ]] || exit 1
  if [[ $1 == stop ]];then printf 'inactive\n';else printf 'active\n';fi > "$UKE_MOTION_RUNNING";;
 *) exit 91;;
esac
SYSTEMD
cat > "$work/bin/busctl" <<'BUSCTL'
#!/usr/bin/bash
set -Eeuo pipefail
[[ $* == '--system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig' ]]
printf 'busctl reload-config\n' >> "$UKE_MOTION_CALLS"
[[ ${UKE_MOTION_FAIL:-none} != bus ]]
BUSCTL
chmod 700 "$work/bin/"*
export UKE_MOTION_CALLS=$work/calls UKE_MOTION_RUNNING=$work/running
export UKE_MOTION_ENABLED=static UKE_MOTION_ACTIVE=inactive UKE_MOTION_OWNER=uke-control
reset_fixture(){
 rm -rf -- "$work/state";rm -f -- "$work/running" "$work/link"
 printf 'synthetic owned unit\n' > "$work/units/motion.service"
 : > "$work/calls"
}
for enabled in enabled enabled-runtime disabled static indirect masked masked-runtime;do
 for active in active inactive failed;do
  reset_fixture
  export UKE_MOTION_ENABLED=$enabled UKE_MOTION_ACTIVE=$active
  bash "$work/SAVE" 1
  [[ $(stat -c %a "$work/state") == 700 && $(stat -c %a "$work/state/unit") == 600 ]]
  cp "$work/state/unit" "$work/snapshot"
  if bash "$work/SAVE" 1 > "$work/refusal" 2>&1;then exit 1;fi
  cmp "$work/snapshot" "$work/state/unit"
  bash "$work/PREUN_SAVE" 1
  cmp "$work/snapshot" "$work/state/unit"
  bash "$work/POSTUN_RESTORE" 1
  [[ -f $work/state/unit ]]
  if [[ $active == active ]];then [[ $(grep -c '^systemctl stop ' "$work/calls") == 1 ]];
  else ! grep -q '^systemctl stop ' "$work/calls";fi
  bash "$work/RESTORE" 1
  [[ ! -e $work/state ]]
  if [[ $active == active && $enabled != masked* ]];then grep -Fxq 'systemctl start uke-control-motion.service' "$work/calls";
  else ! grep -q '^systemctl start ' "$work/calls";fi
  ! grep -Eq '^systemctl (enable|disable|unmask|mask|preset|restart|try-restart)' "$work/calls"
 done
done
export UKE_MOTION_ENABLED=static UKE_MOTION_ACTIVE=inactive
# Initial install: no pre-existing unit, no wake or preference change.
reset_fixture;rm "$work/units/motion.service"
bash "$work/SAVE" 1;[[ ! -e $work/state && ! -s $work/calls ]]
printf 'new unit\n' > "$work/units/motion.service"
bash "$work/RESTORE" 1
! grep -Eq '^systemctl (start|stop|enable|unmask|preset)' "$work/calls"
# Downgrade and erase: stop before binary removal, never restart a removed unit.
for count in 0 1;do
 reset_fixture;export UKE_MOTION_ACTIVE=active
 bash "$work/PREUN_SAVE" "$count"
 rm "$work/units/motion.service"
 bash "$work/POSTUN_RESTORE" "$count"
 [[ ! -e $work/state ]]
 grep -Fxq 'systemctl stop uke-control-motion.service' "$work/calls"
 ! grep -q '^systemctl start ' "$work/calls"
done
# A mask selected during a transaction wins over the captured active state.
for mask in masked masked-runtime;do
 reset_fixture;export UKE_MOTION_ENABLED=static UKE_MOTION_ACTIVE=active
 bash "$work/SAVE" 1
 UKE_MOTION_ENABLED=$mask bash "$work/RESTORE" 1
 [[ ! -e $work/state ]];! grep -q '^systemctl start ' "$work/calls"
done
for transient in activating deactivating reloading unknown;do
 reset_fixture
 if UKE_MOTION_ACTIVE=$transient bash "$work/SAVE" 1 > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -e $work/state ]];! grep -q '^systemctl stop ' "$work/calls"
done
for alteration in foreign hardlink symlink mode directory extra version future stale malformed;do
 reset_fixture;export UKE_MOTION_ENABLED=static UKE_MOTION_ACTIVE=inactive
 bash "$work/SAVE" 1
 case $alteration in
  foreign) :;;
  hardlink) ln "$work/state/unit" "$work/link";;
  symlink) mv "$work/state/unit" "$work/link";ln -s "$work/link" "$work/state/unit";;
  mode) chmod 644 "$work/state/unit";;
  directory) chmod 755 "$work/state";;
  extra) printf 'foreign\n' > "$work/state/other";;
  version) sed -i '1s/^1 /2 /' "$work/state/unit";;
  future) sed -i "1c1 $(($(date +%s)+60))" "$work/state/unit";;
  stale) sed -i "1c1 $(($(date +%s)-3601))" "$work/state/unit";;
  malformed) printf 'untrusted\n' > "$work/state/unit";;
 esac
 : > "$work/calls"
 if UKE_MOTION_BAD_OWNER=$([[ $alteration == foreign ]] && printf yes || printf no) bash "$work/RESTORE" 1 > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -s $work/calls ]]
 if UKE_MOTION_BAD_OWNER=$([[ $alteration == foreign ]] && printf yes || printf no) bash "$work/PREUN_SAVE" 1 > "$work/refusal" 2>&1;then exit 1;fi
 [[ ! -s $work/calls ]]
done
reset_fixture
if UKE_MOTION_OWNER=foreign bash "$work/SAVE" 1 > "$work/refusal" 2>&1;then exit 1;fi
[[ ! -e $work/state && ! -s $work/calls ]]
reset_fixture;export UKE_MOTION_ACTIVE=active
if UKE_MOTION_FAIL=stop bash "$work/SAVE" 1 > "$work/refusal" 2>&1;then exit 1;fi
[[ -f $work/state/unit ]]
bash "$work/RESTORE" 1
reset_fixture;bash "$work/SAVE" 1
if UKE_MOTION_FAIL=start bash "$work/RESTORE" 1 > "$work/refusal" 2>&1;then exit 1;fi
[[ -f $work/state/unit ]]
reset_fixture;bash "$work/SAVE" 1
if UKE_MOTION_FAIL=bus bash "$work/RESTORE" 1 > "$work/refusal" 2>&1;then exit 1;fi
[[ -f $work/state/unit ]];! grep -q '^systemctl start ' "$work/calls"
printf 'Motion package hooks:21 preference/activity combinations,initial install,upgrade,downgrade,erase,new masks and malformed/ownership/actor failures pass; private actors only.\n'
