#!/usr/bin/env bash
# Host-only checks for the shell-compatible environment.d assignment.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
config=${1:-$project/packaging/mesa/30-uke-adreno732-interop.conf}
[[ $(grep -c '^FD_DEV_FEATURES=' "$config") == 1 ]]
(
    unset FD_DEV_FEATURES
    source "$config"
    [[ $FD_DEV_FEATURES == enable_tp_ubwc_flag_hint=0 ]]
    FD_DEV_FEATURES=max_sets=4
    source "$config"
    [[ $FD_DEV_FEATURES == max_sets=4:enable_tp_ubwc_flag_hint=0 ]]
    FD_DEV_FEATURES=enable_tp_ubwc_flag_hint=1
    source "$config"
    [[ $FD_DEV_FEATURES == enable_tp_ubwc_flag_hint=1:enable_tp_ubwc_flag_hint=0 ]]
)
printf '%s\n' 'Uke session hint assignment preserves other options and appends the shared value; host only'
