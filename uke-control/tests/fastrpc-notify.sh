#!/usr/bin/bash
# Native COPR only. Never execute this fixture on Uke.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 2 ]]
fixture=$(realpath "$1")
adapter=$(realpath "$2")
work=$(mktemp -d /tmp/uke-fastrpc-notify.XXXXXX)
trap 'find "$work" -depth -delete' EXIT
for mode in root sensors fallback failure repeat disabled wrong-scope no-socket missing-library;do
 mkdir "$work/$mode"
 timeout 15 env LD_PRELOAD="$adapter" "$fixture" "$mode" "$work/$mode"
done
printf 'Original Fedora RTLD_LOCAL listener/stubs:registered-only READY,root/sensors/fallback,original errors/errno,once-only and disabled/missing-library refusals pass;no DSP calls.\n'
