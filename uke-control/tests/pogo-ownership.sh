#!/usr/bin/bash
# Exercise the actual RPM fragment with private state and command interposers.
set -Eeuo pipefail
[[ $# == 1 || $# == 2 ]]
root=$(realpath "$1")
work=$(mktemp -d)
bus_pid=
cleanup(){
 if [[ -n $bus_pid ]];then kill "$bus_pid" 2>/dev/null || :;fi
 find "$work" -depth -delete
}
trap cleanup EXIT
mkdir -p "$work/bin" "$work/run/systemd/system" "$work/run/user/1000" "$work/model"
export POGO_FIXTURE=$work
bus_pid=$(dbus-daemon --session --fork --address="unix:path=$work/run/user/1000/bus" --print-pid=1)
[[ $bus_pid =~ ^[1-9][0-9]*$ ]]
cat > "$work/bin/id" <<'SH'
#!/usr/bin/bash
[[ $# == 1 && $1 == -u ]] || exit 1
printf '0\n'
SH
cat > "$work/bin/rpm" <<'SH'
#!/usr/bin/bash
[[ $# == 4 && $1 == -qf && $2 == --qf && $3 == '%{NAME}' ]] || exit 1
[[ ! -e $POGO_FIXTURE/model/fresh ]] || exit 1
if [[ -e $POGO_FIXTURE/model/folded ]];then printf 'uke-control';else printf '%s' "${4##*/}";fi
SH
cat > "$work/bin/stat" <<'SH'
#!/usr/bin/bash
if [[ $# == 3 && $1 == -c && $2 == %u && $3 == "$POGO_FIXTURE/"* ]];then printf '0\n';else exec /usr/bin/stat "$@";fi
SH
cat > "$work/bin/systemctl" <<'SH'
#!/usr/bin/bash
set -Eeuo pipefail
scope=system
if [[ ${1-} == --user ]];then
 [[ ${2-} == --machine=1000@.host ]] || exit 1
 scope=user-1000;shift 2
fi
command=${1:?};shift
printf '%s %s\n' "$scope" "$command" >> "$POGO_FIXTURE/model/calls"
if [[ $command == daemon-reload ]];then [[ $# == 0 ]];exit;fi
case "$scope" in system) unit=uke-pogo-auth.service;;user-1000) unit=uke-pogo-desktop.service;;esac
[[ ${!#} == "$unit" ]] || exit 1
base=$POGO_FIXTURE/model/$scope
case "$command" in
 is-enabled) cat "$base.enabled";[[ $(cat "$base.enabled") != disabled ]];;
 is-active) cat "$base.active";[[ $(cat "$base.active") == active ]];;
 enable) if [[ ${1-} == --runtime ]];then printf 'enabled-runtime\n' > "$base.enabled";else printf 'enabled\n' > "$base.enabled";fi;;
 start|try-restart)
  [[ ! -e $POGO_FIXTURE/model/fail-start ]] || exit 1
  printf 'active\n' > "$base.active";;
 *) exit 1;;
esac
SH
chmod 700 "$work/bin/"*
sed -e "s|/run/uke-control-pogo-migration|$work/run/state|g" \
 -e "s|/run/systemd/system|$work/run/systemd/system|g" \
 -e "s|/run/user/|$work/run/user/|g" \
 -e "s|/usr/bin/id|$work/bin/id|g" -e "s|/usr/bin/rpm|$work/bin/rpm|g" \
 -e "s|/usr/bin/stat|$work/bin/stat|g" -e "s|/usr/bin/systemctl|$work/bin/systemctl|g" \
 "$root/packaging/pogo-ownership.sh" > "$work/fragment"
run_phase(){ phase=$1 /usr/bin/bash "$work/fragment"; }
reset_model(){
 [[ ! -e $work/run/state ]]
 printf 'static\n' > "$work/model/system.enabled"
 printf 'inactive\n' > "$work/model/system.active"
 printf 'static\n' > "$work/model/user-1000.enabled"
 printf 'active\n' > "$work/model/user-1000.active"
 : > "$work/model/calls"
}
reset_model
run_phase save
printf 'inactive\n' > "$work/model/user-1000.active"
run_phase restore
[[ $(cat "$work/model/user-1000.active") == active && $(cat "$work/model/system.active") == inactive ]]
grep -Fxq 'user-1000 start' "$work/model/calls"
! grep -Eq '^system (start|try-restart|enable)$' "$work/model/calls"
run_phase restore
reset_model
printf 'inactive\n' > "$work/model/user-1000.active"
run_phase save
run_phase restore
! grep -Eq ' (start|try-restart|enable)$' "$work/model/calls"
reset_model
printf 'activating\n' > "$work/model/user-1000.active"
run_phase save
printf 'inactive\n' > "$work/model/user-1000.active"
run_phase restore
[[ $(cat "$work/model/user-1000.active") == active ]]
reset_model
run_phase save
printf 'deactivating\n' > "$work/model/user-1000.active"
run_phase restore
[[ $(cat "$work/model/user-1000.active") == deactivating ]]
! grep -Eq ' (start|try-restart|enable)$' "$work/model/calls"
reset_model
run_phase save
printf 'masked\n' > "$work/model/user-1000.enabled"
printf 'inactive\n' > "$work/model/user-1000.active"
run_phase restore
[[ $(cat "$work/model/user-1000.enabled") == masked && $(cat "$work/model/user-1000.active") == inactive ]]
! grep -Eq ' (start|try-restart|enable)$' "$work/model/calls"
reset_model
printf 'enabled\n' > "$work/model/user-1000.enabled"
run_phase save
printf 'disabled\n' > "$work/model/user-1000.enabled"
run_phase restore
[[ $(cat "$work/model/user-1000.enabled") == enabled ]]
grep -Fxq 'user-1000 try-restart' "$work/model/calls"
reset_model
touch "$work/model/folded"
run_phase save
run_phase restore
grep -Fxq 'user-1000 try-restart' "$work/model/calls"
unlink "$work/model/folded"
reset_model
printf 'enabled-runtime\n' > "$work/model/user-1000.enabled"
run_phase save
printf 'disabled\n' > "$work/model/user-1000.enabled"
run_phase restore
[[ $(cat "$work/model/user-1000.enabled") == enabled-runtime ]]
reset_model
touch "$work/model/fresh"
run_phase save
[[ ! -e $work/run/state ]]
unlink "$work/model/fresh"
ln -s "$work/model" "$work/run/state"
if run_phase save;then exit 1;fi
if run_phase restore;then exit 1;fi
unlink "$work/run/state"
reset_model
run_phase save
printf 'inactive\n' > "$work/model/user-1000.active"
touch "$work/model/fail-start"
if run_phase restore;then exit 1;fi
[[ -f $work/run/state/complete && -f $work/run/state/user-1000 ]]
unlink "$work/model/fail-start"
run_phase restore
printf 'Pogo ownership: active/static restore,inactive/masked preservation,enabled/runtime state,reinstall refresh,fresh skip,symlink and failed-restore retention pass.\n'
