#!/usr/bin/bash
# Text-only overlay of the admitted cumulative102 boot runtime.
set -Eeuo pipefail
boot=$(realpath "${1:?Prepared boot source}")
overlay=$(cd -- "$(dirname -- "$0")" && pwd)
[[ ! -e $boot/src/boot-health-report.hpp ]]
(
 cd "$boot"
 printf '%s\n' \
  'f42712a5db57ddb140e8376359b9827317a00693ebd5607595f4b8d0b0bb029b  src/kernel-update.cpp' \
  'e1a999fd0741cd26cfb50f0ccc100b7cf2b40f7c85b07b0d506453941671eb9e  src/userdata-update.hpp' | sha256sum -c --strict --quiet -
)
git -C "$boot" apply --check "$overlay/rollback-health-observation.patch"
git -C "$boot" apply "$overlay/rollback-health-observation.patch"
install -m644 "$overlay/boot-health-report.hpp" "$boot/src/"
install -m644 "$overlay/boot-health-report.cpp" "$overlay/boot-health-report.sh" "$boot/tests/"
git -C "$boot" apply --reverse --check "$overlay/rollback-health-observation.patch"
printf 'Boot-health report overlay applied to the exact cumulative runtime; no policy or device command executed.\n'
