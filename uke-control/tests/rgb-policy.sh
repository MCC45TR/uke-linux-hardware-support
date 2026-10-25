#!/usr/bin/env bash
# Check the compiled permission required by the actual hardened service.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
policy=${1:-$root/build/selinux/uke_control_rgb.pp}
cil=$(mktemp)
trap 'rm -f -- "$cil"' EXIT
grep -Fxq 'NoNewPrivileges=yes' "$root/packaging/uke-control-rgb.service"
/usr/libexec/selinux/hll/pp < "$policy" > "$cil"
grep -Eq '^\(allow init_t uke_rgb_t \(process2 \([^)]*nnp_transition([ )])' "$cil" || {
    echo 'Compiled RGB policy lacks the systemd NoNewPrivileges transition' >&2
    exit 1
}
printf '%s\n' 'Compiled RGB policy admits the hardened systemd domain transition: PASS (host policy only)'
