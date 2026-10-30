#!/usr/bin/env bash
# Source-only ownership contract; native request and lifetime tests are separate.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
unit=$root/packaging/uke-control-flashlight.service
grep -Fxq 'DeviceAllow=/dev/uke-torch rw' "$unit"
grep -Fxq 'ProtectKernelTunables=yes' "$unit"
grep -Fxq 'BindsTo=dev-uke\x2dtorch.device' "$unit"
! grep -Eq 'ReadWritePaths=|ExecStopPost=|RuntimeDirectory=|Conflicts=sleep.target' "$unit"
! grep -Eq 'write\(|flock\(|fchmod\(|fchown\(|lseek\(|/sys/' "$root/src/torch.cpp"
! test -e "$root/src/torch-access.cpp"
grep -Fq 'KERNEL=="uke-torch"' "$root/packaging/91-uke-control-torch.rules"
! grep -q 'RUN+=' "$root/packaging/91-uke-control-torch.rules"
! grep -Eq 'uke_torch_brightness_t|uke_torch_runtime_t|relabelto|relabelfrom' "$root/packaging/selinux/uke_control_rgb.te"
awk '/^%pre$/ {pre=1; next} /^%post$/ {pre=0} pre && /torch-access --remove/ {found=1} END {exit !found}' "$root/packaging/uke-control.spec"
printf '%s\n' 'Kernel torch ownership and legacy grant revocation: source contract PASS; no native or device result'
