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

COPR 11332798, source 5468a4ae, passed all four ARM64 compiler modes on
16 November. Its text-only receipt has two verified repository signatures.
The first trials exposed a fixture IPA boundary, strict Clang pointer syntax
and a missing compiler-rt build dependency; warnings and sanitizers stayed
enabled. These results supersede the initial source-only qualification.

Prepared Control23 assembly derives from the complete signed Control22
source package. A whole-recipe inverse preserves all earlier phases and
74 inputs. It adds the setup code to the existing executable and reruns
the four native modes against the actual prepared headers. Unit data stays
disabled by default, and package installation creates no activation marker.

Cumulative Control23, COPR11332853/source028a382e, passed all retained checks
and all four modes against its actual prepared headers. Three runtime RPMs
and the signed source RPM had eight independently verified COPR signatures.
The normal repository DNF upgrade succeeded; installed units passed the
systemd parser, remained disabled and created no setup marker.

On accepted7.3.4-2, a private mount-namespace diagnostic exercised the actual
installed endpoint without reopening completed setup globally. The PC read
status and an untruncated11323-byte kernel-log frame. The console blocked in
poll with unchanged0/0 CPU ticks over a bounded two-second idle observation.
Native finish unbound USB and preserved the saved-disabled preference. The
diagnostic's explicit namespace unmount reported busy; namespace exit released
the mounts, and global markers, claim absence and gadget cleanup were checked
before its private temporary files were removed. This was not an original-unit
sandbox or fresh-image startup test.

Fresh-image startup, service-domain SELinux execution, host reconnection,
longer idle/power measurements, USB bootloader transition and the original
wizard completion path remain pending. Synthetic and namespace results do
not establish that complete lifecycle.
