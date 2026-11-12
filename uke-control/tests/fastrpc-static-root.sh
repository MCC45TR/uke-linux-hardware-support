#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Target software fixture: original sleep replaces the SDK at its mount target.
# No DSP file is opened; production services and their accepted cache stay live.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
[[ $EUID == 0 && $# == 1 && $(getenforce) == Enforcing ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
[[ $(uname -r) == 7.3.3-24-unstable ]]
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-control) == 2.17.0-6.fc46 ]]
[[ $(stat -c '%u:%g:%a' /run/uke-ssc/state) == 0:0:700 ]]
[[ $(stat -c '%u:%g:%a' /usr/share/uke-control) == 0:0:700 ]]
units=(uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target)
mkdir -m700 "$output"
work=$(mktemp -d /usr/share/uke-control/fastrpc-fixture.XXXXXXXX)
name=uke-fastrpc-staticroot-${work##*.}
cleanup() {
 local result=$?
 trap - EXIT
 systemctl stop "$name-baseline.service" "$name-fixed.service" >/dev/null 2>&1 || :
 rm -f -- "/run/systemd/system/$name-baseline.service" "/run/systemd/system/$name-fixed.service"
 systemctl daemon-reload
 systemctl reset-failed "$name-baseline.service" "$name-fixed.service" >/dev/null 2>&1 || :
 find "$work" -depth -delete
 exit "$result"
}
trap cleanup EXIT
trap 'exit 130' INT TERM HUP
trap 'printf "Static-root fixture failed at line %s\n" "$LINENO" >&2' ERR
for unit in "${units[@]}";do
 systemctl show -p ActiveState -p UnitFileState "$unit" > "$output/$unit.before"
 grep -Fqx ActiveState=active "$output/$unit.before"
done
stat -c '%d:%i:%u:%g:%a' /run/uke-ssc/state > "$output/cache-generation.before"
find /run/uke-ssc/state/sensors/persist -type f | wc -l > "$output/cache-count.before"
bash "$owner/packaging/stage-fastrpc-root.sh" "$work/root"
restorecon -RF "$work"
stat -c '%C' "$work/root" > "$output/root-context.txt"
grep -Fqx system_u:object_r:usr_t:s0 "$output/root-context.txt"
for mode in baseline fixed;do
 node --input-type=module - "$owner/src/fastrpc/uke-ssc-root.service" "$work/root" "$mode" "/run/systemd/system/$name-$mode.service" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';
const [source,root,mode,out]=process.argv.slice(2);
let text=fs.readFileSync(source,'utf8').split('[Service]\n')[1];assert(text);
function once(a,b){assert.equal(text.split(a).length,2,a);text=text.replace(a,()=>b);}
once('Type=notify\n','Type=exec\n');once('NotifyAccess=main\n','NotifyAccess=none\n');
once('Environment=UKE_FASTRPC_NOTIFY=listener-v1 LD_PRELOAD=/usr/lib64/uke-control/libuke-fastrpc-notify.so ADSP_LIBRARY_PATH=/usr/lib/qcom/adsp\n','');
once('fastrpc-session root\n','fastrpc-session 10\n');
once('RootDirectory=/usr/share/uke-control/fastrpc-root\n','RootDirectory='+root+'\n');
once(' /usr/libexec/uke-control/fastrpc-session /run/systemd/notify ',' /usr/bin/sleep:/usr/libexec/uke-control/fastrpc-session /run/systemd/notify ');
assert.equal(text.split('/run/uke-ssc/fedora/state').length,7);
text=text.replaceAll('/run/uke-ssc/fedora/state','/run/uke-ssc/state');
once('/run/uke-ssc/fedora/registry.lock:','/run/uke-ssc/registry.lock:');
once('Restart=on-failure\n','Restart=no\nRuntimeMaxSec=15\n');
assert.equal(text.split('BindLogSockets=no\n').length,2);
if(mode==='baseline')once('BindLogSockets=no\n','');else assert.equal(mode,'fixed');
fs.writeFileSync(out,'[Unit]\nDescription=Uke public static-root software fixture\n[Service]\n'+text);
JS
done
systemctl daemon-reload
status=0;systemctl start "$name-baseline.service" > "$output/baseline-start.txt" 2>&1 || status=$?
[[ $status != 0 ]]
systemctl show "$name-baseline.service" -p Result -p ExecMainStatus > "$output/baseline-result.txt"
grep -Fqx ExecMainStatus=226 "$output/baseline-result.txt"
systemctl start "$name-fixed.service" > "$output/fixed-start.txt" 2>&1
[[ $(systemctl show -p ActiveState --value "$name-fixed.service") == active ]]
pid=$(systemctl show -p MainPID --value "$name-fixed.service")
# Type=exec acknowledges flock's first exec, before its --no-fork child exec.
# Match the original sleep inode after that bounded software-only transition.
for attempt in {1..20};do
 [[ $(stat -Lc '%d:%i' /proc/$pid/exe) == $(stat -c '%d:%i' /usr/bin/sleep) ]] && break
 sleep .05
done
readlink /proc/$pid/exe > "$output/fixed-exe.txt"
[[ $(stat -Lc '%d:%i' /proc/$pid/exe) == $(stat -c '%d:%i' /usr/bin/sleep) ]]
for socket in dev-log socket stdout;do
 [[ ! -e /proc/$pid/root/run/systemd/journal/$socket ]]
done
[[ ! -e /proc/$pid/root/dev/log ]]
[[ ! -e /proc/$pid/root/proc/1 && ! -e /proc/$pid/root/sys/class/remoteproc ]]
[[ -S /proc/$pid/root/run/systemd/notify ]]
[[ $(systemctl show -p BindLogSockets --value "$name-fixed.service") == no ]]
[[ $(systemctl show -p MemorySwapCurrent --value "$name-fixed.service") == 0 ]]
[[ $(stat -c '%d:%i' /proc/$pid/root/persist/sensors/registry) == $(stat -c '%d:%i' /run/uke-ssc/state/sensors/persist) ]]
systemctl stop "$name-fixed.service"
for unit in "${units[@]}";do
 systemctl show -p ActiveState -p UnitFileState "$unit" | cmp - "$output/$unit.before"
done
stat -c '%d:%i:%u:%g:%a' /run/uke-ssc/state | cmp - "$output/cache-generation.before"
find /run/uke-ssc/state/sensors/persist -type f | wc -l | cmp - "$output/cache-count.before"
[[ $(getenforce) == Enforcing ]]
printf 'usr_t static root: implicit journal-bind baseline226;explicit no succeeds with original sleep,hidden journal/proc/sys,notify socket,zero swap;production states and cache retained.\n' > "$output/result.txt"
touch "$output/complete"
