# Temporary USB shutdown trace

1 November 2026: version 1.8.20 corrects early observer cleanup seen in 1.8.18.
Native Fedora Rawhide AArch64 checks pass. During an ordinary POCO Pad X1
restart, the observer remains until user.slice stops and exits normally.
No USB fault is diagnosed in that capture. Version1.8.21 retains this ordering.

The DNF transaction starts a current-session diagnostic. It observes `usb_role_switch_set_role`,
`dwc3_set_mode` and `dwc3_host_exit` using the kernel's existing kprobe interface.
Role/mode arguments and up to 32 kernel stacks per probe go to the system
journal under `uke-shutdown-trace`. Raw traces are private engineering evidence.

The service uses its own trace instance and 64 KiB per CPU. It preserves
the global tracer and foreign probes, refuses an existing instance/group and
removes only its own state on exit. Setup failure also performs cleanup. It
does not write USB roles, PHY settings, firmware or boot/storage metadata. The
unit stops after `user.slice` and before `umount.target`, retaining the trace
through user-process shutdown. It has no ordering after `multi-user.target`,
which would create a cycle through logind. It runs from `/` and does not hold
the external project volume. Offline checks inspect cycle warnings and deleted
jobs as well as exit status; systemd can return zero after breaking a cycle.
The package fixture projects these ordering edges into a private user-manager
graph. Full installed-system ordering and physical shutdown are separate checks.

The package does not enable the diagnostic at boot or request a reboot. This
temporary diagnostic must be retired after the disconnect call path is known.
It does not repair the disconnect or qualify shutdown reliability. Read its
status with `systemctl status uke-shutdown-trace.service` and its records with
`journalctl -u uke-shutdown-trace.service`.

The interfaces follow the upstream Linux [kprobe event](https://docs.kernel.org/trace/kprobetrace.html)
and [trace instance](https://docs.kernel.org/trace/ftrace.html) documentation.
No OEM board settings or donor scripts are used. Missing kernel facilities,
symbols or denied trace access stop the diagnostic without changing USB state.
