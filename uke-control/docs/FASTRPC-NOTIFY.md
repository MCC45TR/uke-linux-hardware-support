# Original Fedora FastRPC readiness integration

Source candidate,12 November2026. This is an existing-Control integration
candidate,not a Fedora rebuild or a new runtime package. It has not been
delivered to Uke or accepted as complete SSC parity.

The original Fedora1.0.8 listener registers with the DSP after executable
startup. Existing Uke root/sensor units wait for that registered-listener
boundary. A private preload forwards the two exported registration functions
to the already loaded original library and sends systemd READY only after a
successful return. Return values and errno remain unchanged. It has no worker,
constructor,timer,polling loop,file mapping or DSP ioctl. Opt-in is scoped to
the owning service by UKE_FASTRPC_NOTIFY=listener-v1 and NOTIFY_SOCKET.

adsprpcd loads its listener RTLD_LOCAL. The adapter uses RTLD_NOLOAD and a
specific library handle to preserve that scope,without loading or promoting a
second listener. The exact original Fedora ELF has both register calls through
its PLT and no SYMBOLIC binding. Missing originals must not publish readiness.

The native fixture loads unmodified Fedora listener code and generated stubs
with the daemon's local scope. Only remote RPC/device-name operations are
synthetic; it checks root/sensors/fallback,failure,errno,ordering,idempotence,
disabled scope and a missing original. No real DSP device is opened. Passing
this fixture does not establish a live provider switch or namespace/registry,
SELinux,crash/suspend and private-RAM/read-only-persist parity.

The implementation is MIT project integration. Protocol remains in original
[Qualcomm FastRPC1.0.8](https://github.com/qualcomm/fastrpc/tree/d247519650fe5cb16de6c78edaa95bcc4be25073),
BSD-3-Clause. Relevant files are src/dsprpcd.c,src/adsp_default_listener.c and
the two generated default-listener stubs. Current development8f592739 retains
this boundary; the reviewed21 open pull requests do not supply this readiness
notification. Return path:remove this adapter when an original Fedora listener
provides the same registered-state systemd notification. Hardware protocol and
lifecycle changes must still use native kernel/DT ownership where applicable.
