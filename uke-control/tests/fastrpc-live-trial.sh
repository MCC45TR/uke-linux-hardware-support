#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Bounded qualification, never a persistent provider installation.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
units=(uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target)
consumers=(uke-adaptive-display.target iio-sensor-proxy.service uke-ssc-sensors.service uke-ssc-root.service)

observe_samples() {
 local destination=$1 datatype property
 for datatype in accel gyro ambient_light ambient_light_back_strm; do
  timeout --signal=TERM --kill-after=1 8 /usr/libexec/uke-control/ssc-sample "$datatype" 2>/dev/null |
   awk -v datatype="$datatype" '
    $1 != datatype || $2 !~ /^timestamp_ticks=[0-9]+$/ || $3 !~ /^status=[0-3]$/ || $4 !~ /^data=/ {bad=1;next}
    {d=NF-3;s=substr($3,8)+0;if(!n){dimensions=d;low=s;high=s}
     if(d!=dimensions || d<1)bad=1;if(s<low)low=s;if(s>high)high=s;n++}
    END {if(bad || !n)exit 1;printf "%s samples=%d dimensions=%d min_status=%d max_status=%d\n",datatype,n,dimensions,low,high}'
 done > "$destination"
}

verify_provider() {
 local service pid
 for service in uke-ssc-root.service uke-ssc-sensors.service; do
  [[ $(systemctl show -p ActiveState --value "$service") == active ]]
  pid=$(systemctl show -p MainPID --value "$service")
  grep -Fq /usr/lib64/libadsp_default_listener.so.1.0.0 "/proc/$pid/maps"
  grep -Fq /usr/lib64/libadsprpc.so.1.0.0 "/proc/$pid/maps"
  [[ $(readlink "/proc/$pid/exe") == /usr/libexec/uke-control/fastrpc-session ]]
  printf '%s %s\n' "$service" "$pid"
 done
}

restore() {
 local work=$1 unit active enabled current
 [[ $work =~ ^/run/uke-fastrpc-live\.[[:alnum:]]{8}$ ]]
 [[ -d $work && ! -L $work && $(stat -c '%u:%g:%a' "$work") == 0:0:700 ]]
 exec 8>"$work/recovery.lock"
 flock --exclusive 8
 [[ ! -e $work/restored ]] || return 0
 if [[ -e $work/software-only ]]; then
  systemctl stop "$(cat "$work/dummy-unit").service"
  touch "$work/restored"
  return 0
 fi
 [[ -f $work/expected && $(wc -l < "$work/expected") == 5 ]]
 systemctl stop "${consumers[@]}"
 for unit in uke-ssc-root.service uke-ssc-sensors.service; do
  rm -f -- "/run/systemd/system/$unit.d/90-$(basename "$work").conf"
 done
 systemctl daemon-reload
 systemctl reset-failed uke-ssc-root.service uke-ssc-sensors.service
 while read -r unit active enabled; do
  case "$unit" in uke-ssc-cache.service|uke-ssc-root.service|uke-ssc-sensors.service|iio-sensor-proxy.service|uke-adaptive-display.target) :;; *) return 1;; esac
  [[ $active == active && ( $enabled == static || $enabled == disabled ) ]]
  current=$(systemctl is-enabled "$unit") || :
  case "$current" in masked|masked-runtime) continue;; esac
  systemctl start "$unit"
 done < "$work/expected"
 if mountpoint --quiet "$work/ram"; then umount "$work/ram"; fi
 # No private data was ever placed outside the noswap mount.
 find "$work/root" -depth -delete
 while read -r unit active enabled; do
  current=$(systemctl is-enabled "$unit") || :
  # A deliberate new mask wins over the saved active state.
  case "$current" in masked|masked-runtime) continue;; esac
  [[ $current == "$enabled" ]]
  [[ $(systemctl show -p ActiveState --value "$unit") == "$active" ]]
 done < "$work/expected"
 touch "$work/restored"
}

if [[ ${1-} == --restore ]]; then
 [[ $EUID == 0 && $# == 2 ]]
 restore "$2"
 exit
fi
[[ $EUID == 0 && $# == 3 && ( $1 == live || $1 == recovery ) ]]
mode=$1
output=$(realpath -m "$2")
[[ ! -e $output && ! -L $output ]]
exec 9>"$3"
flock --exclusive --nonblock 9
mkdir -m700 "$output"
work=$(mktemp -d /run/uke-fastrpc-live.XXXXXXXX)
unit=$(basename "$work")
install -m500 "$(realpath "$0")" "$work/trial.sh"
printf '%s\n' "$work" > "$output/work-path.txt"
armed=false
finish() {
 local result=$?
 trap - EXIT INT TERM HUP
 if [[ $armed == true ]]; then
  if /usr/bin/bash "$work/trial.sh" --restore "$work" > "$output/restore.log" 2>&1; then
   systemctl stop "$unit-recovery.timer" >/dev/null 2>&1 || :
   systemctl stop "$unit-recovery.service" >/dev/null 2>&1 || :
   systemctl reset-failed "$unit-recovery.service" >/dev/null 2>&1 || :
   touch "$output/restored"
  else
   printf 'Immediate restore failed; independent recovery timer retained.\n' >&2
   result=1
  fi
 elif mountpoint --quiet "$work/ram"; then
  umount "$work/ram"
 fi
 if [[ -e $work/restored || $armed == false ]]; then
  find "$work" -depth -delete
 fi
 exit "$result"
}
trap finish EXIT
trap 'exit 130' INT TERM HUP

if [[ $mode == recovery ]]; then
 touch "$work/software-only"
 printf '%s\n' "$unit-dummy" > "$work/dummy-unit"
 systemd-run --quiet --unit="$unit-dummy" -p RuntimeMaxSec=15 -- /usr/bin/sleep 15
 systemd-run --quiet --unit="$unit-recovery" --on-active=2s --timer-property=AccuracySec=100ms \
  -p Type=oneshot -p TimeoutStartSec=15 -- /usr/bin/bash "$work/trial.sh" --restore "$work"
 armed=true
 # This is only a public software timer fixture; production units are untouched.
 sleep 4
 [[ -e $work/restored ]]
 [[ $(systemctl show -p ActiveState --value "$unit-dummy.service") == inactive ]]
 printf 'Independent timer stopped its public dummy and completed recovery.\n' > "$output/result.txt"
 touch "$output/complete"
 exit
fi

[[ $(uname -r) == 7.3.3-24-unstable ]]
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' uke-control) == 2.17.0-6.fc46 ]]
[[ $(rpm -q --qf '%{VERSION}-%{RELEASE}' fastrpc) == 1.0.8-1.fc46 ]]
! rpm -q fastrpc-services >/dev/null 2>&1
[[ $(getenforce) == Enforcing ]]
findmnt --mountpoint /mnt/vendor/persist --options ro --noheadings >/dev/null
[[ -c /dev/fastrpc-adsp && -c /dev/dma_heap/system ]]
[[ $(stat -c '%u:%g:%a' /run/uke-ssc/state) == 0:0:700 ]]
[[ $(du -sb /run/uke-ssc/state | cut -f1) -lt 4194304 ]]
[[ $(find /run/uke-ssc/state -type l | wc -l) == 0 ]]
printf '%s  %s\n' \
 896c1aaf13ecfc0e18029c18bab9f801acc764ac8af9d432a340522fa1c50f64 /usr/lib64/libadsp_default_listener.so.1.0.0 \
 f7b8e3fa9dba51b913ba8c9c6e82227f8749fcbbd67228e4a11bd57e1635347d /usr/lib64/libadsprpc.so.1.0.0 | sha256sum -c --strict --quiet -
for service in "${units[@]}"; do
 active=$(systemctl show -p ActiveState --value "$service")
 enabled=$(systemctl is-enabled "$service") || :
 [[ $active == active && ( $enabled == static || $enabled == disabled ) ]]
 printf '%s %s %s\n' "$service" "$active" "$enabled" >> "$work/expected"
done
cp "$work/expected" "$output/units-before.txt"
for rproc in /sys/class/remoteproc/remoteproc*; do
 [[ $(cat "$rproc/crash_count") == 0 ]]
 printf '%s %s %s\n' "$(cat "$rproc/name")" "$(cat "$rproc/state")" "$(cat "$rproc/crash_count")"
done > "$output/dsp-before.txt"
stat -c '%i' /run/uke-ssc/state > "$output/cache-inode-before.txt"
find /run/uke-ssc/state/sensors/persist/registry -type f -printf '.\n' | wc -l > "$output/registry-count-before.txt"
observe_samples "$output/samples-legacy.txt"
mkdir -p "$work/ram" "$work/root"/{usr/bin,usr/lib64,usr/libexec/uke-control,usr/lib/qcom/adsp,run/systemd,run/uke-ssc,dev/dma_heap,sys/devices/soc0,proc,mnt/vendor,persist/sensors/registry,vendor/etc/sensors/config,odm/etc/sensors/config,system,tmp,var/tmp}
for alias in bin lib64; do ln -s "usr/$alias" "$work/root/$alias"; done
ln -s usr/lib "$work/root/lib"
ln -s /persist "$work/root/mnt/vendor/persist"
ln -s /vendor "$work/root/system/vendor"
mkdir -p "$work/root/usr/lib"
touch "$work/root/usr/bin/flock" "$work/root/usr/lib/ld-linux-aarch64.so.1" \
 "$work/root/usr/libexec/uke-control/fastrpc-session" "$work/root/run/systemd/notify" \
 "$work/root/run/uke-ssc/registry.lock" "$work/root/dev/fastrpc-adsp" "$work/root/dev/dma_heap/system" \
 "$work/root/vendor/etc/sensors/sns_reg_config"
mount -t tmpfs -o size=16M,nr_inodes=1024,mode=0700,noswap,nosuid,nodev,noexec tmpfs "$work/ram"
findmnt --mountpoint "$work/ram" --noheadings --output FSTYPE,OPTIONS > "$output/ram-options.txt"
grep -Eq '^tmpfs[[:space:]].*(^|,)noswap(,|$)' "$output/ram-options.txt"
for service in uke-ssc-root.service uke-ssc-sensors.service; do
 mkdir -p "/run/systemd/system/$service.d"
 [[ ! -e /run/systemd/system/$service.d/90-$unit.conf ]]
done
systemd-run --quiet --unit="$unit-recovery" --on-active=75s --timer-property=AccuracySec=1s \
 -p Type=oneshot -p TimeoutStartSec=120 -- /usr/bin/bash "$work/trial.sh" --restore "$work"
armed=true
systemctl is-active --quiet "$unit-recovery.timer"
systemctl stop "${consumers[@]}"
# Old cache remains the accepted generation; only this small copy can change.
flock --exclusive --timeout 5 /run/uke-ssc/registry.lock \
 cp -a --reflink=never /run/uke-ssc/state "$work/ram/state"
touch "$work/ram/registry.lock"
chmod 600 "$work/ram/registry.lock"
for session in root sensors; do
 service=uke-ssc-$session.service
 cat > "/run/systemd/system/$service.d/90-$unit.conf" <<EOF
[Service]
Type=notify
NotifyAccess=main
ExecCondition=
ExecStart=
ExecStart=/usr/bin/flock --shared --no-fork /run/uke-ssc/registry.lock /usr/libexec/uke-control/fastrpc-session $session
Environment=UKE_FASTRPC_NOTIFY=listener-v1 LD_PRELOAD=/usr/lib64/uke-control/libuke-fastrpc-notify.so ADSP_LIBRARY_PATH=/usr/lib/qcom/adsp
RootDirectory=$work/root
WorkingDirectory=/
RuntimeDirectory=
# Either empty bind setting clears both lists; reset once before all entries.
BindReadOnlyPaths=
BindReadOnlyPaths=/usr/lib64 /usr/lib/ld-linux-aarch64.so.1 /usr/bin/flock /usr/libexec/uke-control/fastrpc-session /run/systemd/notify $work/ram/state/sensors/config:/vendor/etc/sensors/config $work/ram/state/sensors/config:/odm/etc/sensors/config $work/ram/state/sensors/sns_reg.conf:/vendor/etc/sensors/sns_reg_config $work/ram/state/dsp/adsp:/usr/lib/qcom/adsp $work/ram/state/socinfo:/sys/devices/soc0
BindPaths=$work/ram/state/sensors/persist:/persist/sensors/registry $work/ram/registry.lock:/run/uke-ssc/registry.lock /dev/fastrpc-adsp /dev/dma_heap/system
ReadOnlyPaths=
ReadWritePaths=
ReadWritePaths=/persist/sensors/registry /run/uke-ssc/registry.lock
TemporaryFileSystem=/sys:ro /proc:ro
MountAPIVFS=no
PrivateMounts=yes
PrivateDevices=yes
PrivateTmp=yes
ProtectSystem=strict
ProtectHome=yes
NoNewPrivileges=yes
CapabilityBoundingSet=
RestrictAddressFamilies=AF_UNIX
MemoryDenyWriteExecute=yes
MemorySwapMax=0
DeviceAllow=
DevicePolicy=closed
DeviceAllow=/dev/fastrpc-adsp rw
DeviceAllow=/dev/dma_heap/system rw
Restart=no
RuntimeMaxSec=60
TimeoutStartSec=20
TimeoutStopSec=5
TimeoutStopFailureMode=terminate
LimitCORE=0
StandardOutput=null
StandardError=null
EOF
done
systemctl daemon-reload
for service in uke-ssc-root.service uke-ssc-sensors.service; do
 # Inspect the parsed contract, rather than trusting the rendered text alone.
 readonly_binds=$(systemctl show -p BindReadOnlyPaths --value "$service")
 [[ $readonly_binds == *'/usr/lib64'* && $readonly_binds == *'/usr/bin/flock'* && $readonly_binds == *'/run/systemd/notify'* ]]
done
for service in uke-ssc-root.service uke-ssc-sensors.service; do
 status=0
 systemctl start "$service" > "$output/start-$service.log" 2>&1 || status=$?
 systemctl show -p ActiveState -p SubState -p Result -p ExecMainStatus -p MainPID -p NRestarts "$service" >> "$output/provider-result.txt"
 [[ $status == 0 && $(systemctl show -p ActiveState --value "$service") == active ]]
done
verify_provider > "$output/pids-before-restart.txt"
systemctl start iio-sensor-proxy.service uke-adaptive-display.target
observe_samples "$output/samples-fedora.txt"
awk '{print $1,$3}' "$output/samples-legacy.txt" > "$output/dimensions-legacy.txt"
awk '{print $1,$3}' "$output/samples-fedora.txt" > "$output/dimensions-fedora.txt"
cmp "$output/dimensions-legacy.txt" "$output/dimensions-fedora.txt"
systemctl restart uke-ssc-root.service
systemctl start uke-ssc-sensors.service iio-sensor-proxy.service uke-adaptive-display.target
verify_provider > "$output/pids-after-restart.txt"
awk 'NR==FNR {old[$1]=$2;next} $1 in old && old[$1]!=$2 {n++} END{exit n!=2}' \
 "$output/pids-before-restart.txt" "$output/pids-after-restart.txt"
observe_samples "$output/samples-restarted.txt"
awk '{print $1,$3}' "$output/samples-restarted.txt" > "$output/dimensions-restarted.txt"
cmp "$output/dimensions-legacy.txt" "$output/dimensions-restarted.txt"
for property in HasAccelerometer HasAmbientLight; do
 [[ $(busctl get-property --system net.hadess.SensorProxy /net/hadess/SensorProxy net.hadess.SensorProxy "$property") == 'b true' ]]
 printf '%s=true\n' "$property"
done > "$output/proxy-properties.txt"
systemctl show -p CPUUsageNSec -p MemoryCurrent -p MemorySwapCurrent uke-ssc-root.service uke-ssc-sensors.service > "$output/idle-before.txt"
sleep 10
for service in "${units[@]}"; do systemctl is-active --quiet "$service"; done
systemctl show -p CPUUsageNSec -p MemoryCurrent -p MemorySwapCurrent uke-ssc-root.service uke-ssc-sensors.service > "$output/idle-after.txt"
for rproc in /sys/class/remoteproc/remoteproc*; do
 printf '%s %s %s\n' "$(cat "$rproc/name")" "$(cat "$rproc/state")" "$(cat "$rproc/crash_count")"
done > "$output/dsp-after.txt"
cmp "$output/dsp-before.txt" "$output/dsp-after.txt"
stat -c '%i' /run/uke-ssc/state > "$output/cache-inode-after.txt"
cmp "$output/cache-inode-before.txt" "$output/cache-inode-after.txt"
find /run/uke-ssc/state/sensors/persist/registry -type f -printf '.\n' | wc -l > "$output/registry-count-after.txt"
cmp "$output/registry-count-before.txt" "$output/registry-count-after.txt"
printf 'Original Fedora root/sensors readiness,stationary sample dimensions,manual root restart with fresh PIDs,proxy presence and bounded idle observation passed;automatic restore remains mandatory.\n' > "$output/result.txt"
touch "$output/complete"
