# UKE-SWAP-STARTUP-20261117: retain admission without repeating preparation

17 November 2026. Accepted POCO Pad X1 kernel 7.3.4-8, Control 2.17.0-27,
Fedora Rawhide. The measured systemd startup is 0.620 seconds kernel,
3.055 seconds initrd and 9.654 seconds userspace. Graphical target is reached
8.549 seconds into userspace. These are systemd intervals, not power-to-desktop
optical timing.

The critical path contains Control boot at 2.620 seconds and a second
`swap apply` in uke-swap-ready at 2.312 seconds. Both perform the same complete
logical-swap admission. Status is approximately 8 ms, lsblk 46 ms and the
global blkid scan 584 ms. PTY device timings from blame are not proof of a
critical-path delay. No global timeout, firmware or root-capacity gate is removed.

Control28 prepares the entire existing swap policy and stores successful
admission in a root-private RAM receipt bound to the boot, policy and block
inventory. The session barrier's new ready command reuses only unchanged,
active owned logical swap with matching descriptor/size/signature/label and
a zero DM event count. Manual apply and every unknown/changed state retain
complete admission. The partition-first selector is byte identical, including
unique-source and three-slot ownership refusal and safe managed-file retirement.
The existing local root-capacity ordering remains authoritative.

The actual active swap is a logical mapper inside physical UFS super, not a
GPT partition symlink. Its full device is 9,017,753,600 bytes; active capacity
excludes the 4-KiB swap header. The size is greater than RAM, but kernel
integrity lockdown refuses hibernation and the current sleep policy disables it.
That is a separate secure-resume prerequisite, not a reason to create a file
or bypass lockdown. No physical hibernation acceptance is claimed.

The new source recipe retains all 85 signed parent inputs, every prior native
check and the same runtime package names. Host patch/whole-source/whole-recipe
inverses and shell syntax pass. COPR native fault injection and runtime build,
signed DNF delivery, measured boots and shutdown diagnosis remain pending.
Only prior user-session shutdown logs are available, so no system shutdown
bottleneck is established yet.
