# Serialized first-boot preparation

On a fresh full singleboot installation, the first-setup USB apply operation
can overlap the image's swap-ready service after the main Control boot task.
The nonblocking operation lock previously made one fail. A swap-ready failure
then prevented its dependent Plasma login and first-setup services from starting.

Control26 waits on the existing operation lock when applying the boot swap
policy and when entering or finishing temporary setup USB. Manual CDC requests
retain their nonblocking default. The blocking flock sleeps in the kernel;
there is no retry timer or background polling task. Existing ownership, mode,
hardlink, symlink, storage admission and persistent-policy checks remain intact.

The original Chrony sandbox can also start before the installation profile
directory exists. The existing drop-in now uses systemd StateDirectory to create
that trusted directory before ProtectSystem=strict is applied. Clock restore
and save retain their original admission checks and no RTC write is added.

Native qualification runs the actual production lock with fork/flock contention,
release and waiter cancellation, plus untrusted file refusals, in GCC and Clang
plain/sanitizer builds. Current setup-policy and logical-swap tests run alongside
all retained signed parent checks. This does not establish a successful new
physical cold first boot; package delivery and that acceptance remain separate.
