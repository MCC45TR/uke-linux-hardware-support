#!/usr/bin/bash
# Exercise the actual RPM cache-refresh body with a fake dependency cascade.
set -Eeuo pipefail
data_only=false
if [[ $# == 2 && $1 == --data-only ]];then data_only=true;shift
else test ! -e /etc/senemos/kernel-update.conf;fi
[[ $# == 1 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin" "$work/manager"
printf 'xiaomi,uke\0' > "$work/compatible"
if [[ $data_only == true ]];then sed 's/%%/%/g' "$1" > "$work/spec"
else rpmspec -P "$1" > "$work/spec";fi
awk '
 $0=="# UKE_CONTROL_SSC_REFRESH_BEGIN"{copy=1;begin++;next}
 $0=="# UKE_CONTROL_SSC_REFRESH_END"{copy=0;end++;next}
 copy{print}
 END{if(begin!=1||end!=1)exit 1}' "$work/spec" |
 sed -e "s|/run/systemd/system|$work/manager|g" -e "s|/sys/firmware/devicetree/base/compatible|$work/compatible|g" > "$work/refresh"
bash -n "$work/refresh"
cat > "$work/bin/systemctl" <<'SYSTEMD'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_REFRESH_CALLS"
case "$*" in
 '-q is-active uke-ssc-root.service') [[ $UKE_REFRESH_ROOT == active ]];;
 '-q is-active uke-adaptive-display.target') [[ $(cat "$UKE_REFRESH_CURRENT") == active ]];;
 'is-enabled uke-adaptive-display.target') printf '%s\n' "$UKE_REFRESH_MODE";;
 'daemon-reload') :;;
 'try-restart uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service')
  printf 'inactive\n' > "$UKE_REFRESH_CURRENT"
  [[ $UKE_REFRESH_FAIL == no ]];;
 'start uke-adaptive-display.target')
  [[ $UKE_REFRESH_MODE != masked && $UKE_REFRESH_MODE != masked-runtime ]]
  printf 'active\n' > "$UKE_REFRESH_CURRENT";;
 *) exit 91;;
esac
SYSTEMD
chmod 700 "$work/bin/systemctl"
export PATH=$work/bin:$PATH UKE_REFRESH_CALLS=$work/calls UKE_REFRESH_CURRENT=$work/current
cases=0
for root in active inactive;do
 for target in active inactive;do
  for mode in disabled static masked masked-runtime;do
   printf '%s\n' "$target" > "$work/current";: > "$work/calls"
   UKE_REFRESH_ROOT=$root UKE_REFRESH_MODE=$mode UKE_REFRESH_FAIL=no bash "$work/refresh"
   expected=$target
   if [[ $root == active ]];then
    grep -Fq 'try-restart uke-ssc-cache.service uke-ssc-root.service uke-ssc-sensors.service iio-sensor-proxy.service' "$work/calls"
    if [[ $target == active && $mode != masked && $mode != masked-runtime ]];then
     grep -Fxq 'start uke-adaptive-display.target' "$work/calls"
    else
     ! grep -q '^start ' "$work/calls";expected=inactive
    fi
   else ! grep -Eq '^(daemon-reload|try-restart|start)' "$work/calls";fi
   [[ $(cat "$work/current") == "$expected" ]]
   ! grep -Eq '^(--no-reload.*enable|enable|unmask)' "$work/calls"
   ((cases+=1))
  done
 done
done
printf 'active\n' > "$work/current";: > "$work/calls"
UKE_REFRESH_ROOT=active UKE_REFRESH_MODE=disabled UKE_REFRESH_FAIL=yes bash "$work/refresh" > "$work/refused" 2>&1
grep -Fq 'Uke SSC cache refresh refused' "$work/refused"
! grep -q '^start ' "$work/calls"
printf 'SSC refresh:%s cascade cases and failed refresh refusal passed;active disabled target restored,new masks respected,no enable operation.\n' "$cases"
