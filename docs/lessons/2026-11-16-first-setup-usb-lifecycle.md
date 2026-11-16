# UKE-FIRST-SETUP-USB-20261116: scope USB access to the setup lifecycle

Date: 16 November 2026. Environment: separate PC, accepted POCO Pad X1 kernel
7.3.4-2 and original Fedora Plasma setup. The existing authenticated serial
console has demonstrated commands, bounded kernel logs and an ordinary reboot.
That account-based test does not qualify access before account creation.

Prepared source adds an internal command to the existing Control executable.
A fresh image must opt in with a protected marker. The original manual CDC
helper retains board, profile, API, role and gadget ownership checks. Setup
claims only its temporary activation; it does not change the saved preference.
Completion of the original Plasma wizard closes the restricted endpoint,
restores that preference and disables the temporary setup units.

The endpoint accepts only status, bounded kernel logs and the existing native
bootloader reboot helper. It executes fixed argument vectors, bounds input and
output, checks the exact kernel tty device and rechecks ownership/completion
before commands. There is no shell or network export. Idle waits block on the
UDC sysfs notification or tty input. Linux 7.3.4 UDC core explicitly notifies
the state attribute through `usb_gadget_state_work()`.

The native-only qualification compiles the actual implementation with GCC,
Clang and sanitizers. Private files and a real Linux PTY exercise refusal,
JSON framing, fixed reboot arguments, completion and preference preservation;
gadget, service and reboot actions use asserted stubs. The check package
contains receipts only and is not a tablet runtime dependency.

Only source/text checks have run on the PC. Native compilation, cumulative
Control packaging, signed DNF, SELinux/unit execution, fresh-image startup,
host reconnection, idle measurements and completion cleanup remain pending.
The installed system and its USB policy are unchanged by this source work.
