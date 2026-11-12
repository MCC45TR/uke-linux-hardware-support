# Original Fedora FastRPC in the installed SELinux namespace

The12 November software fixture runs on kernel7.3.3-24-unstable, Control6,
original Fedora systemd262.2-1 and Enforcing SELinux. Its temporary static root
is staged below the production parent with the ordinary usr_t label. Original
Fedora sleep replaces the SDK at its bind target; no FastRPC/DMA device is
opened and production services remain active. The fixture is source-only.

Without an explicit setting, systemd binds the three journal sockets because
PrivateDevices and RootDirectory are set. init_t cannot create their missing
file targets under usr_t; the negative case exits226/NAMESPACE. BindLogSockets=no
starts successfully, leaves journal sockets absent, preserves notification,
the private namespace and zero-swap contract, and retains production states and
cache generation. No SELinux policy or label exception is installed.

The exact default and override are in original
[systemd v262.2 execute.c](https://github.com/systemd/systemd/blob/v262.2/src/core/execute.c),
exec_context_get_effective_bind_log_sockets. The implicit socket list and private
device symlink are in
[namespace.c](https://github.com/systemd/systemd/blob/v262.2/src/core/namespace.c).
The explicit setting precedes the default; MountAPIVFS=no independently prevents
automatic API filesystem/host-release mounts. Original systemd is unchanged.

From the hardware-support checkout, with a new output directory:

```sh
sudo bash uke-control/tests/fastrpc-static-root.sh /var/tmp/uke-fastrpc-static-root
```

The baseline and fixed units have bounded lifetimes and an exit cleanup trap.
The fixed check waits for flock's child exec and matches the original sleep
inode. A Type=exec start alone acknowledges flock's first exec, so an immediate
executable assertion is insufficient. The fixture retains public metadata only.

The separate live-static mode uses the same staged usr_t root with real original
Fedora listeners, a16MiB noswap cache copy and an independent75s recovery timer:

```sh
sudo bash uke-control/tests/fastrpc-live-trial.sh live-static \
 /var/tmp/uke-fastrpc-live-static .kernel-coordination/kernel-delivery.lock
```

Its stationary accel/gyro/dual-light outputs and root restart pass; the following
10s observation has CPU delta0/0 and swap0. It restores accepted6 and removes
the owned static root, private cache and overrides. No motion is required.
These are bounded prerequisites. Candidate11 source/native/signed RPM/DNF,
cold boot, crash/suspend and complete original-provider parity remain separate.
