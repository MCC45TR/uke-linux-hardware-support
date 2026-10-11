#!/bin/sh
# Owner-enabled, root-only USB debug transport; never changes storage or Type-C roles.
set -eu
g=/sys/kernel/config/usb_gadget/uke_debug
cleanup() {
    printf '\n' > "$g/UDC"
    rm -f "$g/configs/c.1/acm.gs0" "$g/configs/c.1/ecm.usb0"
    rmdir "$g/configs/c.1/strings/0x409" "$g/configs/c.1" \
        "$g/functions/acm.gs0" "$g/functions/ecm.usb0" "$g/strings/0x409" "$g"
}
owned() {
    [ "$(cat "$g/idVendor")" = 0x1d6b ] &&
    [ "$(cat "$g/idProduct")" = 0x0104 ] &&
    [ "$(cat "$g/strings/0x409/product")" = 'Uke USB debug' ]
}
case ${1-} in
    stop) [ -d "$g" ] || exit 0; owned; cleanup; exit 0;;
    start) :;;
    *) echo 'Usage: uke-usb-network start|stop' >&2; exit 2;;
esac
uke-boot-status --cdc
[ -d /sys/class/udc/a600000.ssusb ]
if [ -d "$g" ]; then
    owned
    [ "$(cat "$g/UDC")" = a600000.ssusb ]
    [ "$(readlink -f "$g/configs/c.1/acm.gs0")" = "$g/functions/acm.gs0" ]
    [ "$(readlink -f "$g/configs/c.1/ecm.usb0")" = "$g/functions/ecm.usb0" ]
    exit 0
fi
[ ! -d /sys/module/g_serial ] || modprobe -r g_serial
modprobe libcomposite
modprobe usb_f_ecm
modprobe usb_f_acm
mkdir "$g"
trap 'cleanup || :' EXIT
cd "$g"
printf '0x1d6b\n' > idVendor
printf '0x0104\n' > idProduct
printf '0x0200\n' > bcdUSB
printf '0xEF\n' > bDeviceClass
printf '0x02\n' > bDeviceSubClass
printf '0x01\n' > bDeviceProtocol
mkdir strings/0x409 configs/c.1 configs/c.1/strings/0x409
printf 'Senemos\n' > strings/0x409/manufacturer
printf 'Uke USB debug\n' > strings/0x409/product
printf 'CDC ACM + ECM\n' > configs/c.1/strings/0x409/configuration
printf '250\n' > configs/c.1/MaxPower
mkdir functions/acm.gs0 functions/ecm.usb0
printf '02:77:00:00:00:02\n' > functions/ecm.usb0/dev_addr
printf '02:77:00:00:00:01\n' > functions/ecm.usb0/host_addr
ln -s "$g/functions/acm.gs0" configs/c.1/acm.gs0
ln -s "$g/functions/ecm.usb0" configs/c.1/ecm.usb0
printf 'a600000.ssusb\n' > UDC
trap - EXIT
