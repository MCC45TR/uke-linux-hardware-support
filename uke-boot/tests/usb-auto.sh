#!/usr/bin/env bash
# Host-only attachment fixtures for the actual helper functions; no configfs or module writes.
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
root=$work/root
mkdir -p "$root"/{etc/senemos,proc,run/senemos-uke,sys/class/udc,sys/class/usb_role,sys/class/typec}
provider=$root/sys/devices/platform/soc/a600000.ssusb
mkdir -p "$provider/of_node" "$provider/udc/renamed-controller"
printf 'qcom,sm7675-dwc3\0' > "$provider/of_node/compatible"
printf 'peripheral\0' > "$provider/of_node/dr_mode"
ln -s "$provider/udc/renamed-controller" "$root/sys/class/udc/renamed-controller"
printf 'senemos.debug=cdc-acm\n' > "$root/proc/cmdline"
printf 'auto-debug\n' > "$root/etc/senemos/usb-sharing"
mkdir -p "$root/sys/class/power_supply/qcom-battmgr-usb"
psy=$root/sys/class/power_supply/qcom-battmgr-usb
printf '1\n' > "$psy/online"
printf 'Unknown [SDP] DCP CDP PD\n' > "$psy/usb_type"
sed -n '/^full_typec_profile() {/,/^allow_legacy() {/p' "$here/src/uke-usb-network.sh" |
    sed '$d' | sed "s@/sys/@$root/sys/@g; s@/etc/@$root/etc/@g; s@/proc/@$root/proc/@g" > "$work/functions.sh"
# These are project functions copied from the helper, with only filesystem roots redirected.
source "$work/functions.sh"
state=$root/run/senemos-uke
identity_valid=yes
uke-boot-status() { [[ $identity_valid == yes ]]; }
mkdir -p "$provider/role/usb-role-switch"
printf 'device\n' > "$provider/role/usb-role-switch/role"
ln -s "$provider/role/usb-role-switch" "$root/sys/class/usb_role/renamed-role"
cat() {
    case $1 in "$root"/sys/class/power_supply/*)
        : > "$work/power-get-attempted"
        return 1;;
    esac
    command cat "$@"
}
ready() { auto_ready || { echo "Unexpected refusal: $auto_reason" >&2; exit 1; }; }
refused() {
    if auto_ready; then echo "Unexpected admission: $1" >&2; exit 1; fi
    [[ $auto_reason == "$1" ]]
}
refused PC_ATTACHMENT_UNVERIFIED
mkdir -p "$root/sys/class/typec/port7" "$root/sys/class/typec/port7-partner"
printf 'host [device]\n' > "$root/sys/class/typec/port7/data_role"
ready
printf '1\n' > "$state/usb-gadget-inhibit"
refused DEBUG_NOT_AUTHORIZED
rm "$state/usb-gadget-inhibit"
printf 'disabled\n' > "$root/etc/senemos/usb-sharing"
refused POLICY_DISABLED
printf 'auto-debug\n' > "$root/etc/senemos/usb-sharing"
printf 'otg\0' > "$provider/of_node/dr_mode"
rm "$root/sys/class/usb_role/renamed-role"
refused DATA_ROLE_UNVERIFIED
ln -s "$provider/role/usb-role-switch" "$root/sys/class/usb_role/renamed-role"
ready
printf 'none\n' > "$provider/role/usb-role-switch/role"
refused DATA_ROLE_UNVERIFIED
printf 'host\n' > "$provider/role/usb-role-switch/role"
refused DATA_ROLE_UNVERIFIED
printf 'device\n' > "$provider/role/usb-role-switch/role"
printf '[host] device\n' > "$root/sys/class/typec/port7/data_role"
refused NO_PC_PARTNER
printf 'host [device]\n' > "$root/sys/class/typec/port7/data_role"
mkdir "$root/sys/class/typec/port8"
refused PORT_ASSOCIATION_AMBIGUOUS
rmdir "$root/sys/class/typec/port8"
# Full Type-C CDC requires the saved opt-in, never the old early-debug argument.
mkdir -p "$root/sys/firmware/devicetree/base/chosen"
printf 'typec-v1\0' > "$root/sys/firmware/devicetree/base/chosen/senemos,usb-profile"
printf 'root=PARTLABEL=linux\n' > "$root/proc/cmdline"
refused PROFILE_PROVIDER_MISMATCH
printf 'qcom,sm7675-dwc3-typec\0' > "$provider/of_node/compatible"
ready
identity_valid=no
refused DEBUG_NOT_AUTHORIZED
identity_valid=yes
printf 'disabled\n' > "$root/etc/senemos/usb-sharing"
refused POLICY_DISABLED
rm "$root/etc/senemos/usb-sharing"
refused POLICY_DISABLED
printf 'auto-debug\n' > "$root/etc/senemos/usb-sharing"
ready
[[ $(cat "$here/src/profiles/usb-sharing") == disabled ]]
grep -Fxq 'disable uke-usb-auto.service' "$here/src/profiles/80-uke-usb.preset"
! grep -q '^ConditionKernelCommandLine=' "$here/src/units/uke-usb-auto.service"
[[ ! -e $work/power-get-attempted ]]
printf 'USB auto attachment fixtures passed; no hardware operations\n'
