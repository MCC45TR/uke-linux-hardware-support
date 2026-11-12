#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Bounded original Fedora test. Arguments: reference directory, fresh output, delivery lock.
# The private classification models a candidate route; it does not install that route.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
restore() {
 local w=$1 u active enabled current row path load
 [[ $w =~ ^/run/uke-stock-proxy24\.[[:alnum:]]{8}$ && -d $w && ! -L $w ]]
 exec 8>"$w/restore.lock";flock --exclusive 8
 [[ ! -e $w/restored ]] || return 0
 load=$(systemctl show -p LoadState --value "$(basename "$w")-stock.service")
 if [[ $load != not-found ]];then
  systemctl stop "$(basename "$w")-stock.service"
  [[ $(systemctl show -p MainPID --value "$(basename "$w")-stock.service") == 0 ]]
 fi
 for row in "$w"/iio:*;do
  [[ -f $row/path ]] || continue
  path=$(cat "$row/path")
  [[ $path == /sys/bus/iio/devices/iio:device*/buffer* ]]
  printf '0\n' > "$path/enable"
  while IFS=$'\t' read -r key value;do printf '%s\n' "$value" > "${path%/*}/scan_elements/$key";done < "$row/enables"
  cat "$row/length" > "$path/length"
  [[ $(cat "$path/enable") == 0 && $(cat "$path/length") == "$(cat "$row/length")" ]]
  while IFS=$'\t' read -r key value;do [[ $(cat "${path%/*}/scan_elements/$key") == "$value" ]];done < "$row/enables"
 done
 while read -r u active enabled;do
  [[ $active == active ]] || continue
  current=$(systemctl is-enabled "$u") || :
  case "$current" in masked|masked-runtime) continue;;esac
  systemctl start "$u"
 done < "$w/units"
 while read -r u active enabled;do
  current=$(systemctl is-enabled "$u") || :
  case "$current" in masked|masked-runtime) continue;;esac
  [[ $current == "$enabled" && $(systemctl show -p ActiveState --value "$u") == "$active" ]]
 done < "$w/units"
 touch "$w/restored"
}
if [[ ${1-} == --restore ]];then [[ $EUID == 0 && $# == 2 ]];restore "$2";exit;fi
[[ $EUID == 0 && $# == 3 && $(uname -r) == 7.3.3-24-unstable ]]
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-control) == 2.17.0-12.fc46 ]]
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-iio-sensor-proxy) == 3.9-24.fc46 ]]
[[ $(getenforce) == Enforcing ]]
ref=$(realpath "$1")
out=$(realpath -m "$2")
delivery_lock=$(realpath "$3")
[[ -d $ref && ! -e $out && ! -L $out ]]
output_owner=$(stat -c '%u:%g' "$(dirname "$out")")
mkdir -m700 "$out"
rpmkeys -Kv "$ref/original.rpm" > "$out/signatures"
grep -Fq 'd924b10d3e810dabdd8b56b596e7e91491211fce: OK' "$out/signatures"
! grep -Eq 'NOKEY|NOT OK|BAD' "$out/signatures"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$ref/original.rpm") == iio-sensor-proxy-3.9-3.fc45.aarch64 ]]
[[ $(sha256sum "$ref/original/usr/libexec/iio-sensor-proxy" | cut -d' ' -f1) == $(rpm -qp --qf '[%{FILENAMES}\t%{FILEDIGESTS}\n]' "$ref/original.rpm" | awk '$1=="/usr/libexec/iio-sensor-proxy"{print $2}') ]]
exec 9>"$delivery_lock"
flock --exclusive --nonblock 9
w=$(mktemp -d /run/uke-stock-proxy24.XXXXXXXX)
unit=$(basename "$w")
printf '%s\n' "$w" > "$out/work-path"
install -m500 "$0" "$w/trial.sh"
armed=false
finish() {
 local rc=$?
 trap - EXIT INT TERM HUP
 if [[ $armed == true ]];then
  if /usr/bin/bash "$w/trial.sh" --restore "$w" > "$out/restore.log" 2>&1;then
   systemctl stop "$unit-recovery.timer" "$unit-recovery.service" >/dev/null 2>&1 || :
   touch "$out/restored"
  else printf 'Independent restore timer retained.\n' >&2;rc=1;fi
 fi
 if [[ -e $w/restored || $armed == false ]];then
  cp "$w/units" "$out/units-before" 2>/dev/null || :
  find "$w" -depth -delete
 fi
 printf '%s\n' "$rc" > "$out/final.status"
 chown -R "$output_owner" "$out"
 exit "$rc"
}
trap finish EXIT
trap 'exit 130' INT TERM HUP
for u in uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target uke-battery.service uke-offline-charge.service;do
 active=$(systemctl show -p ActiveState --value "$u")
 enabled=$(systemctl is-enabled "$u") || :
 case "$active" in active|inactive) :;;*) exit 1;;esac
 printf '%s %s %s\n' "$u" "$active" "$enabled" >> "$w/units"
done
grep -Fqx 'iio-sensor-proxy.service active static' "$w/units"
mkdir "$w/udev"
cp -a /run/udev/data/. "$w/udev/"
for d in /sys/bus/iio/devices/iio:device*;do
 name=$(cat "$d/name")
 case "$name" in
  uke-ssc-front-light|uke-ssc-rear-light|uke-ssc-proximity)
   dev=$(cat "$d/dev");[[ $dev =~ ^[0-9]+:[0-9]+$ ]]
   [[ -f $w/udev/c$dev ]]
   sed -i '/^E:IIO_SENSOR_PROXY_TYPE=/d' "$w/udev/c$dev"
   printf '%s\n' "$name" >> "$out/private-classification"
   ;;
  uke-ssc-dual-light) printf '%s\n' "$d" > "$out/combined-path";;
  uke-ssc-magnetic-heading) printf '%s\n' "$d" > "$out/heading-path";;
 esac
done
[[ $(wc -l < "$out/private-classification") == 3 && -f $out/combined-path && -f $out/heading-path ]]
cat > "$w/launch.sh" <<'SH'
#!/usr/bin/bash
set -euo pipefail
mount --bind "$1/udev" /run/udev/data
export G_MESSAGES_DEBUG=all
"$2/usr/libexec/iio-sensor-proxy" 2>&1 |
 awk '/Found device .* of type .* at |Buffer .* did not have data within|Unable to enable ring buffer/{print;fflush()}' > "$3/selection"
SH
bash -n "$w/launch.sh"
systemd-run --quiet --unit="$unit-recovery" --on-active=65s --timer-property=AccuracySec=100ms \
 -p Type=oneshot -p TimeoutStartSec=25 -- /usr/bin/bash "$w/trial.sh" --restore "$w"
armed=true
systemctl stop uke-adaptive-display.target iio-sensor-proxy.service
for d in /sys/bus/iio/devices/iio:device*;do
 [[ $(cat "$d/name") == uke-ssc-* ]] || continue
 path=$d/buffer0;[[ -d $path ]] || path=$d/buffer
 [[ -d $path ]] || continue
 [[ $(cat "$path/enable") == 0 ]]
 row=$w/${d##*/};mkdir "$row"
 printf '%s\n' "$path" > "$row/path";cat "$path/length" > "$row/length"
 for e in "$d"/scan_elements/*_en;do printf '%s\t%s\n' "${e##*/}" "$(cat "$e")";done > "$row/enables"
done
systemd-run --quiet --unit="$unit-stock" -p RuntimeMaxSec=45 -p TimeoutStopSec=5 \
 -p StandardOutput=null -p StandardError=null -- \
 /usr/bin/unshare --mount --propagation private /usr/bin/bash "$w/launch.sh" "$w" "$ref/original" "$out"
ready=false
for ((i=0;i<100;i++));do
 if busctl call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus GetConnectionUnixProcessID s net.hadess.SensorProxy > "$out/owner" 2>/dev/null;then ready=true;break;fi
 sleep .1
done
[[ $ready == true ]]
read -r signature pid < "$out/owner"
[[ $signature == u && $pid =~ ^[0-9]+$ ]]
grep -Fq "$unit-stock.service" "/proc/$pid/cgroup"
[[ $(readlink "/proc/$pid/exe") == "$ref/original/usr/libexec/iio-sensor-proxy" ]]
for prop in HasAccelerometer HasAmbientLight HasProximity;do
 [[ $(busctl get-property net.hadess.SensorProxy /net/hadess/SensorProxy net.hadess.SensorProxy "$prop") == 'b true' ]]
 printf '%s=true\n' "$prop" >> "$out/capabilities"
done
compass=$(busctl get-property net.hadess.SensorProxy /net/hadess/SensorProxy/Compass net.hadess.SensorProxy.Compass HasCompass)
case "$compass" in 'b true') compass=1;;'b false') compass=0;;*) exit 1;;esac
printf 'HasCompass=%s\n' "$compass" >> "$out/capabilities"
monitor_args=(--accel --light --proximity)
[[ $compass == 0 ]] || monitor_args+=(--compass)
set +e
timeout --signal=TERM --kill-after=1s 12s "$ref/original/usr/bin/monitor-sensor" "${monitor_args[@]}" 2>&1 |
 awk -v compass="$compass" '
 /^=== Has accelerometer /{a++}
 /^=== Has ambient light sensor /{l++;if($0 !~ /unit: lux/)bad=1}
 /^=== Has proximity sensor /{p++}
 /^=== Has compass \(heading: [0-9]/{c++}
 /Failed to claim|WARNING|CRITICAL/{bad=1}
 END{printf "accelerometer=%d light_lux=%d proximity=%d compass_finite=%d errors=%d\n",a,l,p,c,bad;exit bad||a!=1||l!=1||p!=1||c!=compass}' > "$out/monitor-counts"
statuses=("${PIPESTATUS[@]}")
set -e
printf '%s\n' "${statuses[*]}" > "$out/monitor-status"
[[ ${statuses[0]} == 124 && ${statuses[1]} == 0 ]]
[[ $(grep -c 'Found device ' "$out/selection") == $((3+compass)) ]]
grep -F "$(realpath "$(cat "$out/combined-path")") of type ambient light sensor at IIO Polling Light sensor" "$out/selection" >/dev/null
if [[ $compass == 1 ]];then grep -F "$(realpath "$(cat "$out/heading-path")") of type compass at IIO Buffer Compass" "$out/selection" >/dev/null;fi
grep -F 'of type proximity at Input proximity' "$out/selection" >/dev/null
printf '{"original_fedora":true,"private_udev_view":true,"native_accel_combined_als_proximity_claims":true,"native_compass_accepted":%s,"physical_accuracy_accepted":false,"suspend_accepted":false,"installed_provider_changed":false,"retirement_authorized_by_this_trial":false}\n' "$([[ $compass == 1 ]] && printf true || printf false)" > "$out/observation.json"
cp "$0" "$out/trial-source.sh"
touch "$out/complete"
printf 'Original Fedora proxy24: three native claims passed,compass=%s; restoring accepted provider.\n' "$compass"
