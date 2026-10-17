# Uke Bluetooth integration

17 October 2026. The kernel owns UART, pinctrl, power rails and QCA firmware
transport. BlueZ owns Bluetooth protocols and HID-over-GATT through kernel UHID.
Original KDE consumes the distribution BlueZ interface.

The exact stock Uke NVM has no unique address. The six-byte factory address is
stored in the hidden device-local `persist/bluetooth/.bt_nv.bin` file. A native
oneshot helper reads that file from read-only persist, verifies both board and
controller DT identities and uses the standard BlueZ management API. Kernel
index events determine readiness; its deadline only bounds failure. An already
configured matching address is left intact. Missing/invalid inputs fail closed.
No new long-running Bluetooth daemon, proprietary firmware or unique address
is shipped in the package.

Explicitly enable `uke-bluetooth-address.service` after device-local inputs are
admitted. Its standard `bluetooth.target` dependency also covers controllers
enumerated in the initramfs before the root udev rule is available.

The .4055 RAM laboratory trial completes firmware transfer at 3 Mbit/s, admits
the actual factory address and supports a 5.2 LE mouse after UHID is loaded.
Movement, clicks and wheel operation are physically confirmed. Packaged helper,
cold boot, reconnect and the separate .4056 tuple require their own evidence.
