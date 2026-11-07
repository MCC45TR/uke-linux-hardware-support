# Scheduler selection — proposed uke-control contract

7 November 2026. Target: POCO Pad X1 / Xiaomi Pad 7 (SM7675), Fedora Rawhide AArch64. Design only; no active scheduler controls, RPMs or device acceptance are added by this proposal. Kernel prerequisites are staged separately in the experimental Uke kernel performance profile.

## Model

Keep standard upstream Linux 7.3 EEVDF as the **default** after boot. Supported user-selectable modes are `eevdf`, `lavd`, `p2dq`, and `bore` only when the corresponding runtime components are verified.

- `eevdf`: no sched_ext scheduler attached; if `kernel.sched_bore` exists, set it to `0`. Never assume `CONFIG_EEVDF`.
- `lavd`: load pinned upstream `scx_lavd` through the reviewed Uke-managed systemd unit; refuse if kernel BTF, sched_ext, matching RPM or binary is unavailable. Disable any earlier active SCX unit before starting.
- `p2dq`: same mechanism using pinned `scx_p2dq`; explicitly warn about currently documented CPU-hotplug and cgroup bandwidth limitations.
- `bore`: only available if an exact Linux 7.3-compatible BORE patch was included in the matched kernel and `kernel.sched_bore` exists. Stop SCX first; toggle `kernel.sched_bore=1` and read it back. A BORE patch is not loaded with eBPF. A BORE-capable kernel can expose EEVDF as `sched_bore=0`.

The four modes are one mutually exclusive **effective** choice from the user's perspective. Actual implementation consists of the fair-class BORE toggle and a separately loaded sched_ext policy. Once SCX unloads, fair scheduling automatically resumes; the BORE sysctl determines whether its patch is enabled.

## CLI and shared client API

Proposed commands (not implemented):

```sh
uke-control scheduler list
uke-control scheduler status
sudo uke-control scheduler set eevdf
sudo uke-control scheduler set lavd
sudo uke-control scheduler set p2dq
sudo uke-control scheduler set bore
```

`--json` returns schema `org.uke.control.scheduler.v1` with `requested`, `effective`, `available`, `reason`, `scx`, `bore`, `kernel`, `governors`, `eas`, and `mglru`; unknown readings are null/unavailable, not false. List the four choices with reason-coded availability for each. The GTK, Qt and TUI clients use the **same** versioned `actions` catalog, plan and bounded execution path rather than implementing their own privileged operations.

The frontend shows a scheduler selector in CPU/Performance preferences. Display EEVDF as the recommended default and LAVD, P2DQ and BORE as experimental choices. Disabled choices explain missing BTF, RPM, unsupported ABI, absent BORE patch or a failed scheduler qualification. Show the effective mode and never infer it just from a saved preference.

## Authorization and safety

A dedicated native root-owned helper should perform mode changes under narrowly scoped Polkit/D-Bus policy. The GUI never runs sudo, shell-constructed commands, arbitrary unit names, arbitrary executable paths or unvalidated sysctl keys. Verify that the running image version, installed scheduler RPM metadata and sched_ext kernel ABI match the accepted matrix. Use fixed, project-owned unit names and exact argv. BORE support must be runtime-probed, not assumed from configuration.

Before switching, capture the current effective scheduler. Stop the known active Uke SCX unit, verify the sched_ext state, set the BORE switch if present, start only the requested allowlisted scheduler unit and confirm the resulting state. On failure, stop any newly started unit, disable BORE if present, verify return to fair scheduling, and return an explicit nonzero error with a private diagnostic receipt. Never leave a failed switch reported as successful.

Do not persist experimental selections across reboot by default. An eventual persistence toggle requires boot-time watchdog, stable fallback, rollback and individual scheduler acceptance. Preserve the kernel updater, matched boot and existing recovery policies. Decline changes during unqualified suspend/resume, thermal emergencies or a service lifecycle transition.

## Packaging / verification

Build pinned ARM64 `scx_lavd` and `scx_p2dq` Rust/BPF programs separately from the Linux kernel; enforce compatible `libbpf`, Clang, Rust and `bpftool` versions. Do not download or compile scheduler code from the internet on the tablet. Enable and verify kernel `CONFIG_SCHED_CLASS_EXT=y`, BPF JIT, BTF, `CONFIG_KALLSYMS_ALL=y` and `CONFIG_FUNCTION_TRACER=y`. BTF / sched_ext support alone is not proof that either scheduler can run.

Test JSON/CLI negative cases with fixture roots (no supported kernel, no executable, mismatched binary, missing BORE sysctl, service failure, partial switch and crash recovery). Compare power and responsiveness on both physical tablet panel variants, and verify user-visible selection, status and rollback independently on Qt, GTK and TUI.

References: https://docs.kernel.org/7.3/scheduler/sched-ext.html ; https://github.com/sched-ext/scx ; https://github.com/firelzrd/bore-scheduler .
