#!/usr/bin/env bash
# User-manager fixtures: substitute every device command and omit reset actions.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
runtime=${XDG_RUNTIME_DIR:?Running user manager required}
test -O "$runtime"
stage=$(mktemp -d "$runtime/uke-deadline.XXXXXX")
name=uke-deadline-test-$$
units=$runtime/systemd/user
mkdir -p "$units"
cleanup() {
    systemctl --user stop "$name.service" "$name-fallback.service" "$name-switch.target" >/dev/null 2>&1 || :
    systemctl --user reset-failed "$name.service" "$name-fallback.service" >/dev/null 2>&1 || :
    rm -f -- "$units/$name.service" "$units/$name-fallback.service" "$units/$name-switch.target"
    rm -rf -- "$stage"
    systemctl --user daemon-reload
}
trap cleanup EXIT
cat > "$units/$name.service" <<UNIT
[Unit]
DefaultDependencies=no
OnFailure=$name-fallback.service
Conflicts=$name-switch.target
Before=$name-switch.target
[Service]
$(grep -E '^(Type|TimeoutStartSec|TimeoutStopSec)=' src/units/uke-userdata-deadline.service)
ExecCondition=/usr/bin/test -e $stage/trial
ExecStart=/usr/bin/sleep infinity
RuntimeMaxSec=1s
UNIT
cat > "$units/$name-fallback.service" <<UNIT
[Unit]
$(grep '^DefaultDependencies=' src/units/uke-userdata-deadline-fallback.service)
After=$name.service
[Service]
$(grep -E '^(Type|TimeoutStartSec|TimeoutStopSec)=' src/units/uke-userdata-deadline-fallback.service)
ExecStart=/bin/sh -c 'printf f >> $stage/fallback'
UNIT
printf '[Unit]\nDescription=Inert switch-root fixture\n' > "$units/$name-switch.target"
systemctl --user daemon-reload
! systemctl --user show "$name-fallback.service" -p After --value | grep -wq basic.target
systemctl --user start "$name.service"
test "$(systemctl --user show "$name.service" -p ActiveState --value)" = inactive
test ! -e "$stage/fallback"
touch "$stage/trial"
systemctl --user start "$name.service"
for ((i=0;i<100;++i));do [[ ! -e $stage/fallback ]] || break;sleep 0.05;done
test "$(cat "$stage/fallback")" = f
systemctl --user reset-failed "$name.service"
rm "$stage/fallback"
systemctl --user start "$name.service"
systemctl --user start "$name-switch.target"
test "$(systemctl --user show "$name.service" -p ActiveState --value)" = inactive
test ! -e "$stage/fallback"
printf 'User-manager deadline: stable skip, trial expiry and switch-root cancellation passed; no device or reboot command\n'
